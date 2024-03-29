
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef MELON_BASE_SHA256_H_
#define MELON_BASE_SHA256_H_

#include <cstdint>
#include <string>

namespace melon::base {

    /*!
     * SHA-256 processor without external dependencies.
     */
    class SHA256 {
      public:
        //! construct empty object.
        SHA256();

        //! construct context and process data range
        SHA256(const void *data, uint32_t size);

        //! construct context and process string
        explicit SHA256(const std::string &str);

        //! process more data
        void process(const void *data, uint32_t size);

        //! process more data
        void process(const std::string &str);

        //! digest length in bytes
        static constexpr size_t kDigestLength = 32;

        //! finalize computation and output 32 byte (256 bit) digest
        void finalize(void *digest);

        //! finalize computation and return 32 byte (256 bit) digest
        std::string digest();

        //! finalize computation and return 32 byte (256 bit) digest hex encoded
        std::string digest_hex();

        //! finalize computation and return 32 byte (256 bit) digest upper-case hex
        std::string digest_hex_uc();

      private:
        uint64_t _length;
        uint32_t _state[8];
        uint32_t _curlen;
        uint8_t _buf[64];
    };

    //! process data and return 32 byte (256 bit) digest hex encoded
    std::string sha256_hex(const void *data, uint32_t size);

    //! process data and return 32 byte (256 bit) digest hex encoded
    std::string sha256_hex(const std::string &str);

    //! process data and return 32 byte (256 bit) digest upper-case hex encoded
    std::string sha256_hex_uc(const void *data, uint32_t size);

    //! process data and return 32 byte (256 bit) digest upper-case hex encoded
    std::string sha256_hex_uc(const std::string &str);

}  // namespace melon::base
#endif  // MELON_BASE_SHA256_H_
