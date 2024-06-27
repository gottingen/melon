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
#include <melon/rpc/webui.h>
#include <turbo/strings/str_format.h>
#include <melon/rpc/server.h>
#include <turbo/log/logging.h>
#include <turbo/strings/str_split.h>

namespace melon {

    static const std::pair<std::string_view , std::string_view> mime_type[] ={
            {"001","application/octet-stream"},
            {"323","text/h323"},
            {"907", "drawing/907"},
            {"acp","audio/x-mei-aac"},
            {"aif","audio/aiff"},
            {"aiff", "audio/aiff"},
            {"asa","text/asa"},
            {"asp","text/asp"},
            {"au","audio/basic"},
            {"awf","application/vnd.adobe.workflow"},
            {"bmp","application/x-bmp"},
            {"c4t","application/x-c4t"},
            {"cal","application/x-cals"},
            {"cdf","application/x-netcdf"},
            {"cel","application/x-cel"},
            {"cg4","application/x-g4"},
            {"cit","application/x-cit"},
            {"cml","text/xml"},
            {"cmx","application/x-cmx"},
            {"crl","application/pkix-crl"},
            {"csi","application/x-csi"},
            {"cut","application/x-cut"},
            {"dbm","application/x-dbm"},
            {"dcd","text/xml"},
            {"der","application/x-x509-ca-cert"},
            {"dib","application/x-dib"},
            {"doc","application/msword"},
            {"drw","application/x-drw"},
            {"dwf", "Model/vnd.dwf"},
            {"dwg","application/x-dwg"},
            {"dxf","application/x-dxf"},
            {"emf","application/x-emf"},
            {"ent","text/xml"},
            {"eps","application/x-ps"},
            {"etd","application/x-ebx"},
            {"fax","image/fax"},
            {"fif","application/fractals"},
            {"frm","application/x-frm"},
            {"gbr","application/x-gbr"},
            {"gif","image/gif"},
            {"gp4","application/x-gp4"},
            {"hmr","application/x-hmr"},
            {"hpl","application/x-hpl"},
            {"hrf","application/x-hrf"},
            {"htc","text/x-component"},
            {"html","text/html"},
            {"htx","text/html"},
            {"ico","image/x-icon"},
            {"iff","application/iff"},
            {"igs","application/x-igs"},
            {"img","application/x-img"},
            {"isp","application/x-internet-signup"},
            {"java","java/*"},
            {"jpe","image/jpeg"},
            {"jpeg","image/jpeg"},
            {"jpg","application/x-jpg"},
            {"jsp","text/html"},
            {"tif", "image/tiff"},
            {"301", "application/x-301"},
            {"906", "drawing/906"},
            {"907", "drawing/907"},
            {"a11", "application/x-a11"},
            {"ai", "application/postscript"},
            {"aifc", "audio/aiff"},
            {"anv", "application/x-anv"},
            {"asf", "video/x-ms-asf"},
            {"asx", "video/x-ms-asf"},
            {"avi", "video/avi"},
            {"biz", "text/xml"},
            {"bot", "application/x-bot"},
            {"c90", "application/x-c90"},
            {"cat", "application/vnd.ms-pki.seccat"},
            {"cdr", "application/x-cdr"},
            {"cer", "application/x-x509-ca-cert"},
            {"cgm", "application/x-cgm"},
            {"class", "java/*"},
            {"cmp", "application/x-cmp"},
            {"cot", "application/x-cot"},
            {"crt", "application/x-x509-ca-cert"},
            {"css", "text/css"},
            {"dbf", "application/x-dbf"},
            {"dbx", "application/x-dbx"},
            {"dcx", "application/x-dcx"},
            {"dgn", "application/x-dgn"},
            {"dll", "application/x-msdownload"},
            {"dot", "application/msword"},
            {"dtd", "text/xml"},
            {"dwf", "application/x-dwf"},
            {"dxb", "application/x-dxb"},
            {"edn", "application/vnd.adobe.edn"},
            {"eml", "message/rfc822"},
            {"epi", "application/x-epi"},
            {"eps", "application/x-ps"},
            {"exe", "application/x-msdownload"},
            {"fdf", "application/vnd.fdf"},
            {"fo", "text/xml"},
            {"g4", "application/x-g4"},
            {"gl2", "application/x-gl2"},
            {"hgl", "application/x-hgl"},
            {"hpg", "application/x-hpgl"},
            {"hqx", "application/mac-binhex40"},
            {"hta", "application/hta"},
            {"htm", "text/html"},
            {"htt", "text/webviewhtml"},
            {"icb", "application/x-icb"},
            {"ico", "image/x-icon"},
            {"ig4", "application/x-g4"},
            {"iii", "application/x-iphone"},
            {"ins", "application/x-internet-signup"},
            {"IVF", "video/x-ivf"},
            {"jfif", "image/jpeg"},
            {"jpe", "application/x-jpe"},
            {"js", "application/javascript"},
            {"latex", "application/x-latex"},
            {"ls", "application/x-javascript"},
            {"m1v", "video/x-mpeg"},
            {"m3u", "audio/mpegurl"},
            {"m4e", "video/mpeg4"},
            {"mac", "application/x-mac"},
            {"math", "text/xml"},
            {"mdb", "application/msaccess"},
            {"mht", "message/rfc822"},
            {"mi", "application/x-mi"},
            {"midi", "audio/mid"},
            {"mml", "text/xml"},
            {"mns", "audio/x-musicnet-stream"},
            {"movie", "video/x-sgi-movie"},
            {"mp2", "video/mpeg"},
            {"mp3", "audio/mpeg"},
            {"mpa", "video/mpeg"},
            {"mpe", "video/mpeg"},
            {"mpg", "video/mpeg"},
            {"mpp", "application/vnd.ms-project"},
            {"mps", "video/x-mpeg"},
            {"mpt", "application/vnd.ms-project"},
            {"mpv", "video/x-mpeg"},
            {"mpw", "application/vnd.ms-project"},
            {"mpx", "application/vnd.ms-project"},
            {"mtx", "text/xml"},
            {"mxp", "application/x-mmxp"},
            {"net", "image/pnetvue"},
            {"nrf", "application/x-nrf"},
            {"nws", "message/rfc822"},
            {"odc", "text/x-ms-odc"},
            {"out", "application/x-out"},
            {"p10", "application/pkcs10"},
            {"p12", "application/x-pkcs12"},
            {"p7b", "application/x-pkcs7-certificates"},
            {"p7c", "application/pkcs7-mime"},
            {"p7m", "application/pkcs7-mime"},
            {"p7r", "application/x-pkcs7-certreqresp"},
            {"p7s", "application/pkcs7-signature"},
            {"pc5", "application/x-pc5"},
            {"pci", "application/x-pci"},
            {"pcl", "application/x-pcl"},
            {"pcx", "application/x-pcx"},
            {"pdf", "application/pdf"},
            {"pdx", "application/vnd.adobe.pdx"},
            {"pfx", "application/x-pkcs12"},
            {"pgl", "application/x-pgl"},
            {"pic", "application/x-pic"},
            {"pko", "application/vnd.ms-pki.pko"},
            {"pl", "application/x-perl"},
            {"plg", "text/html"},
            {"pls", "audio/scpls"},
            {"plt", "application/x-plt"},
            {"awf", "application/vnd.adobe.workflow"},
            {"biz", "text/xml"},
            {"bmp", "application/x-bmp"},
            {"bot", "application/x-bot"},
            {"c4t", "application/x-c4t"},
            {"c90", "application/x-c90"},
            {"cal", "application/x-cals"},
            {"cat", "application/vnd.ms-pki.seccat"},
            {"cdf", "application/x-netcdf"},
            {"cdr", "application/x-cdr"},
            {"cel", "application/x-cel"},
            {"cer", "application/x-x509-ca-cert"},
            {"cg4", "application/x-g4"},
            {"cgm", "application/x-cgm"},
            {"cit", "application/x-cit"},
            {"class", "java/*"},
            {"cml", "text/xml"},
            {"cmp", "application/x-cmp"},
            {"cmx", "application/x-cmx"},
            {"cot", "application/x-cot"},
            {"crl", "application/pkix-crl"},
            {"crt", "application/x-x509-ca-cert"},
            {"csi", "application/x-csi"},
            {"css", "text/css"},
            {"cut", "application/x-cut"},
            {"dbf", "application/x-dbf"},
            {"dbm", "application/x-dbm"},
            {"dbx", "application/x-dbx"},
            {"dcd", "text/xml"},
            {"dcx", "application/x-dcx"},
            {"der", "application/x-x509-ca-cert"},
            {"dgn", "application/x-dgn"},
            {"la1","audio/x-liquid-file"},
            {"lar","application/x-laplayer-reg"},
            {"latex","application/x-latex"},
            {"lavs","audio/x-liquid-secure"},
            {"lmsff","audio/x-la-lms"},
            {"ls","application/x-javascript"},
            {"ltr","application/x-ltr"},
            {"m1v","video/x-mpeg"},
            {"m2v","video/x-mpeg"},
            {"m3u","audio/mpegurl"},
            {"m4e","video/mpeg4"},
            {"mac","application/x-mac"},
    };

    const std::string not_found =
            R"(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>404 Not Found</title>
</head>
<body>
<h1>404 Not Found</h1>
</body>
</html>)";

    WebuiConfig WebuiConfig::default_config() {
        WebuiConfig conf;
        conf.mapping_path = "/webui";
        conf.root_path = "/var/www";
        conf.index_path = "index.html";
        conf.not_found_str = not_found;
        conf.not_found_path = "";
        conf.content_types["html"] = "text/html; charset=utf-8";
        conf.content_types["css"] = "text/css; charset=utf-8";
        conf.content_types["js"] = "application/javascript; charset=utf-8";
        conf.content_types["png"] = "image/png";
        conf.content_types["jpg"] = "image/jpeg";
        conf.content_types["jpeg"] = "image/jpeg";
        conf.content_types["gif"] = "image/gif";
        conf.content_types["ico"] = "image/x-icon";
        conf.content_types["svg"] = "image/svg+xml";
        conf.content_types["ttf"] = "font/ttf";
        conf.content_types["woff"] = "font/woff";
        conf.content_types["woff2"] = "font/woff2";
        conf.content_types["eot"] = "font/eot";
        conf.content_types["otf"] = "font/otf";
        conf.content_types["json"] = "application/json";
        conf.content_types["xml"] = "application/xml";
        conf.content_types["pdf"] = "application/pdf";
        conf.content_types["zip"] = "application/zip";
        conf.content_types["tar"] = "application/x-tar";
        conf.content_types["gz"] = "application/x-gzip";
        conf.content_types["bz2"] = "application/x-bzip2";
        conf.content_types["7z"] = "application/x-7z-compressed";
        conf.content_types["rar"] = "application/x-rar-compressed";
        conf.headers["Server"] = "melon";
        conf.headers["Access-Control-Allow-Origin"] = "*";

        return conf;
    }

    std::string WebuiConfig::get_content_type(std::string_view path) const {
        std::vector<std::string_view> segments = turbo::str_split(path, ".", turbo::SkipEmpty());
        if (segments.size() <= 1) {
            return "text/plain";
        }
        auto it = content_types.find(std::string(segments.back()));
        if (it == content_types.end()) {
            return "text/plain";
        }
        return it->second;
    }


    WebuiService::WebuiService() : file_cache_(1024) {

    }

    void WebuiService::impl_method(::google::protobuf::RpcController *controller,
                                   const ::melon::NoUseWebuiRequest *request,
                                   ::melon::NoUseWebuiResponse *response,
                                   ::google::protobuf::Closure *done) {
        (void) request;
        (void) response;
        melon::ClosureGuard done_guard(done);
        auto *ctrl = dynamic_cast<Controller *>(controller);
        const RestfulRequest req(ctrl);
        RestfulResponse resp(ctrl);
        auto file_path = get_file_meta(&req);

        if (!mutil::PathExists(file_path)) {
            process_not_found(&req, &resp);
        } else {
            auto content = get_content(file_path);
            if (content) {
                resp.set_status_code(200);
                resp.set_body(*content);
                resp.set_content_type(conf_.get_content_type(file_path.value()));
            } else {
                process_not_found(&req, &resp);
            }
        }
        // process headers
        for (auto &it: conf_.headers) {
            resp.set_header(it.first, it.second);
        }
    }

    turbo::Status WebuiService::register_server(const WebuiConfig &conf, turbo::Nonnull<Server *> server) {
        std::unique_lock lock(mutex_);
        if (registered_) {
            return turbo::already_exists_error("already registered");
        }
        conf_ = conf;
        if (conf_.mapping_path.empty()) {
            return turbo::invalid_argument_error("mapping_path is empty");
        }
        std::string map_str = turbo::str_format("%s/* => impl_method", conf_.mapping_path.c_str());
        auto r = server->AddService(this,
                                    melon::SERVER_DOESNT_OWN_SERVICE,
                                    map_str);
        if (r != 0) {
            return turbo::internal_error("register restful service failed");
        }
        registered_ = true;
        return turbo::OkStatus();
    }

    mutil::FilePath WebuiService::get_file_meta(const RestfulRequest *request) {
        auto &unresolved_path = request->unresolved_path();
        if (unresolved_path.empty()) {
            return mutil::FilePath(turbo::str_cat(conf_.root_path, "/", conf_.index_path));
        } else {
            return mutil::FilePath(turbo::str_cat(conf_.root_path, "/", unresolved_path));
        }
    }

    std::shared_ptr<std::string> WebuiService::get_content(const mutil::FilePath &path) {
        auto &fpath = path.value();
        std::shared_ptr<std::string> content;
        {
            std::shared_lock lock(file_cache_mutex_);
            auto it = file_cache_.try_get(fpath);
            if (it.second) {
                content = it.first;
            }
        }
        if (content) {
            return content;
        }
        mutil::ScopedFILE fp(fopen(fpath.c_str(), "r"));
        if (!fp) {
            return nullptr;
        }
        fseek(fp.get(), 0, SEEK_END);
        size_t size = ftell(fp.get());
        fseek(fp.get(), 0, SEEK_SET);
        std::string file_content(size, '\0');
        auto r = fread(&file_content[0], 1, size, fp.get());
        if (r != size) {
            return nullptr;
        }
        {
            std::unique_lock lock(file_cache_mutex_);
            file_cache_.put(fpath, file_content);
        }
        return std::make_shared<std::string>(file_content);
    }

    void WebuiService::process_not_found(const RestfulRequest *request, RestfulResponse *response) {
        response->set_status_code(404);
        response->set_content_type("text/html");
        response->set_body(conf_.not_found_str);

        if (conf_.not_found_path.empty()) {
            response->set_body(conf_.not_found_str);
            return;
        }
        auto path = mutil::FilePath(turbo::str_cat(conf_.root_path, "/", conf_.not_found_path));
        auto content = get_content(path);
        if (content) {
            response->set_body(*content);
        } else {
            response->set_body(conf_.not_found_str);
        }
    }
}  // namespace melon
