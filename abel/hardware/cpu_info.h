// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com


#ifndef ABEL_HARDWARE_CPU_INFO_H_
#define ABEL_HARDWARE_CPU_INFO_H_

#include <string_view>
#include <string>
#include "abel/base/profile.h"

namespace abel {
class cpu_info {
  public:

    /**
     *
     */
    cpu_info();

    enum cpu_arch {
        PENTIUM,
        SSE,
        SSE2,
        SSE3,
        SSSE3,
        SSE41,
        SSE42,
        AVX,
        MAX_INTEL_MICRO_ARCHITECTURE
    };

    // Accessors for CPU information.
    std::string_view vendor_name() const { return cpu_vendor_; }

    int signature() const { return signature_; }

    int stepping() const { return stepping_; }

    int model() const { return model_; }

    int family() const { return family_; }

    int type() const { return type_; }

    int extended_model() const { return ext_model_; }

    int extended_family() const { return ext_family_; }

    bool has_mmx() const { return has_mmx_; }

    bool has_sse() const { return has_sse_; }

    bool has_sse2() const { return has_sse2_; }

    bool has_sse3() const { return has_sse3_; }

    bool has_ssse3() const { return has_ssse3_; }

    bool has_sse41() const { return has_sse41_; }

    bool has_sse42() const { return has_sse42_; }

    bool has_avx() const { return has_avx_; }

    // has_avx_hardware returns true when AVX is present in the CPU. This might
    // differ from the value of |has_avx()| because |has_avx()| also tests for
    // operating system support needed to actually call AVX instuctions.
    // Note: you should never need to call this function. It was added in order
    // to workaround a bug in NSS but |has_avx()| is what you want.
    bool has_avx_hardware() const { return has_avx_hardware_; }

    bool has_aesni() const { return has_aesni_; }

    bool has_non_stop_time_stamp_counter() const {
        return has_non_stop_time_stamp_counter_;
    }

    cpu_arch get_cpu_arch() const;

    std::string_view cpu_brand() const { return cpu_brand_; }

  private:
    void initialize();

    int signature_;
    int type_;
    int family_;
    int model_;
    int stepping_;
    int ext_model_;
    int ext_family_;
    bool has_mmx_;
    bool has_sse_;
    bool has_sse2_;
    bool has_sse3_;
    bool has_ssse3_;
    bool has_sse41_;
    bool has_sse42_;
    bool has_avx_;
    bool has_avx_hardware_;
    bool has_aesni_;
    bool has_non_stop_time_stamp_counter_;
    std::string cpu_vendor_;
    std::string cpu_brand_;
};
}  // namespace abel

#endif  // ABEL_HARDWARE_CPU_INFO_H_
