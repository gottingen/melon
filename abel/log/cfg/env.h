// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include "abel/log/cfg/helpers.h"
#include "abel/log/details/registry.h"
#include "abel/log/details/os.h"

//
// Init levels and patterns from env variables LOG_LEVEL
// Inspired from Rust's "env_logger" crate (https://crates.io/crates/env_logger).
// Note - fallback to "info" level on unrecognized levels
//
// Examples:
//
// set global level to debug:
// export LOG_LEVEL=debug
//
// turn off all logging except for logger1:
// export LOG_LEVEL="off,logger1=debug"
//

// turn off all logging except for logger1 and logger2:
// export LOG_LEVEL="off,logger1=debug,logger2=info"

namespace abel {
namespace cfg {
void load_env_levels() {
    auto env_val = details::os::getenv("LOG_LEVEL");
    auto levels = helpers::extract_levels(env_val);
    details::registry::instance().update_levels(std::move(levels));
}

} // namespace cfg
}  // namespace abel
