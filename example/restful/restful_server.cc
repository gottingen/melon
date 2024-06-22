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
// Created by jeff on 24-6-14.
//

#include <melon/rpc/restful_service.h>
#include <melon/rpc/webui.h>
#include <melon/rpc/server.h>

DEFINE_int32(port, 8018, "TCP Port of this server");
DEFINE_int32(idle_timeout_s, -1, "Connection will be closed if there is no "
                                 "read/write operations during the last `idle_timeout_s'");

DEFINE_string(certificate, "cert.pem", "Certificate file path to enable SSL");
DEFINE_string(private_key, "key.pem", "Private key file path to enable SSL");
DEFINE_string(ciphers, "", "Cipher suite used for SSL connections");

namespace myservice {

class NotFoundProcessor : public melon::RestfulProcessor {
private:
    void process(const melon::RestfulRequest *request, melon::RestfulResponse *response) override {
        auto path = request->unresolved_path();
        response->set_status_code(404);
        response->set_header("Content-Type", "text/plain");
        response->set_body("not found\n");
        response->append_body("Request path: ");
        response->append_body(path);
        response->append_body("\n");
    }
};

    class RootProcessor : public melon::RestfulProcessor {
    private:
        void process(const melon::RestfulRequest *request, melon::RestfulResponse *response) override {
            auto path = request->unresolved_path();
            response->set_status_code(200);
            response->set_header("Content-Type", "text/plain");
            response->set_body("I am  root\n");
            response->append_body("\n");
        }
    };

    class PathProcessor : public melon::RestfulProcessor {
    private:
        void process(const melon::RestfulRequest *request, melon::RestfulResponse *response) override {
            auto path = request->unresolved_path();
            response->set_status_code(200);
            response->set_header("Content-Type", "text/plain");
            response->set_body("hala restful\n");
            response->append_body("Request path: ");
            response->append_body(path);
            response->append_body("\n");
        }
    };

}  // namespace myservice

int main(int argc, char* argv[]) {
    google::ParseCommandLineFlags(&argc, &argv, true);
    turbo::setup_color_stderr_sink();
    // Generally you only need one Server.
    melon::Server server;
    auto service = melon::RestfulService::instance();
    service->set_mapping_path("/ea");
    service->set_not_found_processor(std::make_shared<myservice::NotFoundProcessor>())
    ->set_root_processor(std::make_shared<myservice::RootProcessor>())
    ->set_processor("/path", std::make_shared<myservice::PathProcessor>())
    ->set_processor("path1//", std::make_shared<myservice::PathProcessor>())
    ->set_processor("///path3//path0/", std::make_shared<myservice::PathProcessor>());
    auto rs = service->register_server(&server);
    if(!rs.ok()) {
        LOG(ERROR) << "register server failed: " << rs;
        return -1;
    }
    melon::WebuiConfig conf = melon::WebuiConfig::default_config();
    conf.mapping_path = "/ea/ui";
    conf.root_path = "www";
    auto * webui = melon::WebuiService::instance();
    rs = webui->register_server(conf, &server);
    if(!rs.ok()) {
        LOG(ERROR) << "register server failed: " << rs;
        return -1;
    }
    melon::ServerOptions options;
    options.idle_timeout_sec = FLAGS_idle_timeout_s;
    options.mutable_ssl_options()->default_cert.certificate = FLAGS_certificate;
    options.mutable_ssl_options()->default_cert.private_key = FLAGS_private_key;
    options.mutable_ssl_options()->ciphers = FLAGS_ciphers;
    if (server.Start(FLAGS_port, &options) != 0) {
        LOG(ERROR) << "Fail to start HttpServer";
        return -1;
    }
    server.RunUntilAskedToQuit();
    return 0;
}