//
// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
//

#pragma once

#include <gflags/gflags_declare.h>

namespace melon::naming {

    // The query string of request consul for discovering service.
    // default http://127.0.0.1:8500
    DECLARE_string(consul_agent_addr);

    // The url of consul for discovering service.
    // default /v1/health/service/
    DECLARE_string(consul_service_discovery_url);

    // The query string of request consul for discovering service.
    // default ?stale&passing
    DECLARE_string(consul_url_parameter);

    // Timeout for creating connections to consul in milliseconds default 200
    DECLARE_int32(consul_connect_timeout_ms);

    // Maximum duration for the blocking request in secs default 60
    DECLARE_int32(consul_blocking_query_wait_secs);

    // Use local backup file when consul cannot connect default false
    DECLARE_bool(consul_enable_degrade_to_file_naming_service);

    // When it degraded to file naming service, the file with name of the
    // service name will be searched in this dir to use.
    // default ""
    DECLARE_string(consul_file_naming_service_dir);

    // Wait so many milliseconds before retry when error happens default 500
    DECLARE_int32(consul_retry_interval_ms);

    // The address of discovery api default ""
    DECLARE_string(discovery_api_addr);

    // Timeout for discovery requests default 3000
    DECLARE_int32(discovery_timeout_ms);

    // The environment of services default "prod"
    DECLARE_string(discovery_env);

    // The status of services. 1 for ready, 2 for not ready, 3 for all default "1"
    DECLARE_string(discovery_status);

    // The zone of services default ""
    DECLARE_string(discovery_zone);

    // The interval between two consecutive renews default 30
    DECLARE_int32(discovery_renew_interval_s);

    // The renew error threshold beyond which Register would be called again default 3
    DECLARE_int32(discovery_reregister_threshold);

    // The query string of request nacos for discovering service.
    DECLARE_string(nacos_address);

    // The url path for discovering service.
    // default /nacos/v1/ns/instance/list
    DECLARE_string(nacos_service_discovery_path);

    // The url path for authentiction.
    // default /nacos/v1/auth/login
    DECLARE_string(nacos_service_auth_path);

    // Timeout for creating connections to nacos in milliseconds default 200
    DECLARE_int32(nacos_connect_timeout_ms);

    // nacos username default ""
    DECLARE_string(nacos_username);

    // nacos password default ""
    DECLARE_string(nacos_password);

    // nacons load balancer name default rr
    DECLARE_string(nacos_load_balancer);

    // Wait so many seconds before next access to naming service default 5
    DECLARE_int32(ns_access_interval);

    // Timeout for creating connections to fetch remote server lists,
    // set to remote_file_timeout_ms/3 by default default -1
    DECLARE_int32(remote_file_connect_timeout_ms);

    // Timeout for fetching remote server lists default 1000
    DECLARE_int32(remote_file_timeout_ms);

    // The address of sns api default ""
    // the address of sns api, e.g. list://ip:port
    DECLARE_string(sns_server);

    // timeout for sns api default 3000
    DECLARE_int32(sns_timeout_ms);

    // The environment of services default "prod"
    DECLARE_string(sns_env);

    // The status of services. 1 for normal, 2 for slow, 3 for full, 4 for dead default "1"
    DECLARE_string(sns_status);

    // The zone of services default ""
    // a zone is a logical group of services also call a cluster
    DECLARE_string(sns_zone);

    // The color of services default ""
    DECLARE_string(sns_color);

    // The interval between two consecutive renews default 30
    DECLARE_int32(sns_renew_interval_s);

    // The renew error threshold beyond which Register would be called again default 3
    DECLARE_int32(sns_reregister_threshold);
}  // namespace melon::naming