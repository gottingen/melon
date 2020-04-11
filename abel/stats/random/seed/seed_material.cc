//

#include <abel/stats/random/seed/seed_material.h>

#include <fcntl.h>

#ifndef _WIN32

#include <unistd.h>

#else
#include <io.h>
#endif

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <abel/log/abel_logging.h>
#include <abel/asl/ascii.h>
#include <abel/strings/escaping.h>
#include <abel/asl/string_view.h>
#include <abel/strings/strip.h>

#if defined(__native_client__)

#include <nacl/nacl_random.h>
#define ABEL_RANDOM_USE_NACL_SECURE_RANDOM 1

#elif defined(_WIN32)

#include <windows.h>
#define ABEL_RANDOM_USE_BCRYPT 1
#pragma comment(lib, "bcrypt.lib")

#elif defined(__Fuchsia__)
#include <zircon/syscalls.h>

#endif

#if defined(ABEL_RANDOM_USE_BCRYPT)
#include <bcrypt.h>

#ifndef BCRYPT_SUCCESS
#define BCRYPT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif
// Also link bcrypt; this can be done via linker options or:
// #pragma comment(lib, "bcrypt.lib")
#endif

namespace abel {

    namespace random_internal {
        namespace {

// Read OS Entropy for random number seeds.
// TODO(abel-team): Possibly place a cap on how much entropy may be read at a
// time.

#if defined(ABEL_RANDOM_USE_BCRYPT)

            // On Windows potentially use the BCRYPT CNG API to read available entropy.
            bool read_seed_material_from_os_entropyImpl(abel::span<uint32_t> values) {
              BCRYPT_ALG_HANDLE hProvider;
              NTSTATUS ret;
              ret = BCryptOpenAlgorithmProvider(&hProvider, BCRYPT_RNG_ALGORITHM,
                                                MS_PRIMITIVE_PROVIDER, 0);
              if (!(BCRYPT_SUCCESS(ret))) {
                ABEL_RAW_ERROR("Failed to open crypto provider.");
                return false;
              }
              ret = BCryptGenRandom(
                  hProvider,                                             // provider
                  reinterpret_cast<UCHAR*>(values.data()),               // buffer
                  static_cast<ULONG>(sizeof(uint32_t) * values.size()),  // bytes
                  0);                                                    // flags
              BCryptCloseAlgorithmProvider(hProvider, 0);
              return BCRYPT_SUCCESS(ret);
            }

#elif defined(ABEL_RANDOM_USE_NACL_SECURE_RANDOM)

            // On NaCL use nacl_secure_random to acquire bytes.
            bool read_seed_material_from_os_entropyImpl(abel::span<uint32_t> values) {
              auto buffer = reinterpret_cast<uint8_t*>(values.data());
              size_t buffer_size = sizeof(uint32_t) * values.size();

              uint8_t* output_ptr = buffer;
              while (buffer_size > 0) {
                size_t nread = 0;
                const int error = nacl_secure_random(output_ptr, buffer_size, &nread);
                if (error != 0 || nread > buffer_size) {
                  ABEL_RAW_ERROR"Failed to read secure_random seed data: {}", error);
                  return false;
                }
                output_ptr += nread;
                buffer_size -= nread;
              }
              return true;
            }

#elif defined(__Fuchsia__)

            bool read_seed_material_from_os_entropyImpl(abel::span<uint32_t> values) {
              auto buffer = reinterpret_cast<uint8_t*>(values.data());
              size_t buffer_size = sizeof(uint32_t) * values.size();
              zx_cprng_draw(buffer, buffer_size);
              return true;
            }

#else

// On *nix, read entropy from /dev/urandom.
            bool read_seed_material_from_os_entropyImpl(abel::span<uint32_t> values) {
                const char kEntropyFile[] = "/dev/urandom";

                auto buffer = reinterpret_cast<uint8_t *>(values.data());
                size_t buffer_size = sizeof(uint32_t) * values.size();

                int dev_urandom = open(kEntropyFile, O_RDONLY);
                bool success = (-1 != dev_urandom);
                if (!success) {
                    return false;
                }

                while (success && buffer_size > 0) {
                    int bytes_read = read(dev_urandom, buffer, buffer_size);
                    int read_error = errno;
                    success = (bytes_read > 0);
                    if (success) {
                        buffer += bytes_read;
                        buffer_size -= bytes_read;
                    } else if (bytes_read == -1 && read_error == EINTR) {
                        success = true;  // Need to try again.
                    }
                }
                close(dev_urandom);
                return success;
            }

#endif

        }  // namespace

        bool read_seed_material_from_os_entropy(abel::span<uint32_t> values) {
            assert(values.data() != nullptr);
            if (values.data() == nullptr) {
                return false;
            }
            if (values.empty()) {
                return true;
            }
            return read_seed_material_from_os_entropyImpl(values);
        }

        void mix_into_seed_material(abel::span<const uint32_t> sequence,
                                 abel::span<uint32_t> seed_material) {
            // Algorithm is based on code available at
            // https://gist.github.com/imneme/540829265469e673d045
            constexpr uint32_t kInitVal = 0x43b0d7e5;
            constexpr uint32_t kHashMul = 0x931e8875;
            constexpr uint32_t kMixMulL = 0xca01f9dd;
            constexpr uint32_t kMixMulR = 0x4973f715;
            constexpr uint32_t kShiftSize = sizeof(uint32_t) * 8 / 2;

            uint32_t hash_const = kInitVal;
            auto hash = [&](uint32_t value) {
                value ^= hash_const;
                hash_const *= kHashMul;
                value *= hash_const;
                value ^= value >> kShiftSize;
                return value;
            };

            auto mix = [&](uint32_t x, uint32_t y) {
                uint32_t result = kMixMulL * x - kMixMulR * y;
                result ^= result >> kShiftSize;
                return result;
            };

            for (const auto &seq_val : sequence) {
                for (auto &elem : seed_material) {
                    elem = mix(elem, hash(seq_val));
                }
            }
        }

        abel::optional<uint32_t> get_salt_material() {
            // Salt must be common for all generators within the same process so read it
            // only once and store in static variable.
            static const auto salt_material = []() -> abel::optional<uint32_t> {
                uint32_t salt_value = 0;

                if (random_internal::read_seed_material_from_os_entropy(
                        make_span(&salt_value, 1))) {
                    return salt_value;
                }

                return abel::nullopt;
            }();

            return salt_material;
        }

    }  // namespace random_internal

}  // namespace abel
