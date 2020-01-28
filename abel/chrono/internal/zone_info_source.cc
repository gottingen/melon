// Copyright 2016 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   https://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.

#include <abel/chrono/internal/zone_info_source.h>
#include <abel/base/profile.h>

namespace abel {
namespace chrono_internal {

// Defined out-of-line to avoid emitting a weak vtable in all TUs.
zone_info_source::~zone_info_source() {}
std::string zone_info_source::version() const { return std::string(); }

}  // namespace chrono_internal

}  // namespace abel

namespace abel {

namespace chrono_internal {

namespace {

// A default for zone_info_source_factory, which simply
// defers to the fallback factory.
std::unique_ptr<abel::chrono_internal::zone_info_source> DefaultFactory(
    const std::string& name,
    const std::function<
        std::unique_ptr<abel::chrono_internal::zone_info_source>(
            const std::string& name)>& fallback_factory) {
  return fallback_factory(name);
}

}  // namespace

// A "weak" definition for zone_info_source_factory.
// The user may override this with their own "strong" definition (see
// zone_info_source.h).
#if !defined(__has_attribute)
#define __has_attribute(x) 0
#endif
// MinGW is GCC on Windows, so while it asserts __has_attribute(weak), the
// Windows linker cannot handle that. Nor does the MinGW compiler know how to
// pass "#pragma comment(linker, ...)" to the Windows linker.
#if (__has_attribute(weak) || defined(__GNUC__)) && !defined(__MINGW32__)
ZoneInfoSourceFactory zone_info_source_factory __attribute__((weak)) =
    DefaultFactory;
#elif defined(_MSC_VER) && !defined(__MINGW32__) && !defined(_LIBCPP_VERSION)
extern ZoneInfoSourceFactory zone_info_source_factory;
extern ZoneInfoSourceFactory default_factory;
ZoneInfoSourceFactory default_factory = DefaultFactory;
#if defined(_M_IX86)
#pragma comment( \
    linker,      \
    "/alternatename:?zone_info_source_factory@chrono_internal@abel@@3P6A?AV?$unique_ptr@VZoneInfoSource@chrono_internal@abel@@U?$default_delete@VZoneInfoSource@chrono_internal@abel@@@std@@@std@@ABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@5@ABV?$function@$$A6A?AV?$unique_ptr@VZoneInfoSource@chrono_internal@abel@@U?$default_delete@VZoneInfoSource@chrono_internal@abel@@@std@@@std@@ABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@@Z@5@@ZA=?default_factory@chrono_internal@abel@@3P6A?AV?$unique_ptr@VZoneInfoSource@chrono_internal@abel@@U?$default_delete@VZoneInfoSource@chrono_internal@abel@@@std@@@std@@ABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@5@ABV?$function@$$A6A?AV?$unique_ptr@VZoneInfoSource@chrono_internal@abel@@U?$default_delete@VZoneInfoSource@chrono_internal@abel@@@std@@@std@@ABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@@Z@5@@ZA")
#elif defined(_M_IA_64) || defined(_M_AMD64) || defined(_M_ARM64)
#pragma comment( \
    linker,      \
    "/alternatename:?zone_info_source_factory@chrono_internal@abel@@3P6A?AV?$unique_ptr@VZoneInfoSource@chrono_internal@abel@@U?$default_delete@VZoneInfoSource@chrono_internal@abel@@@std@@@std@@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@5@AEBV?$function@$$A6A?AV?$unique_ptr@VZoneInfoSource@chrono_internal@abel@@U?$default_delete@VZoneInfoSource@chrono_internal@abel@@@std@@@std@@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@@Z@5@@ZEA=?default_factory@chrono_internal@abel@@3P6A?AV?$unique_ptr@VZoneInfoSource@chrono_internal@abel@@U?$default_delete@VZoneInfoSource@chrono_internal@abel@@@std@@@std@@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@5@AEBV?$function@$$A6A?AV?$unique_ptr@VZoneInfoSource@chrono_internal@abel@@U?$default_delete@VZoneInfoSource@chrono_internal@abel@@@std@@@std@@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@@Z@5@@ZEA")
#else
#error Unsupported MSVC platform
#endif  // _M_<PLATFORM>
#else
// Make it a "strong" definition if we have no other choice.
ZoneInfoSourceFactory zone_info_source_factory = DefaultFactory;
#endif

}  // namespace chrono_internal

}  // namespace abel
