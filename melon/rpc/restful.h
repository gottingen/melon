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



#ifndef MELON_RPC_RESTFUL_H_
#define MELON_RPC_RESTFUL_H_

#include <string>
#include <melon/utility/strings/string_piece.h>
#include <melon/rpc/server.h>


namespace melon {

    struct RestfulMethodPath {
        std::string service_name;
        std::string prefix;
        std::string postfix;
        bool has_wildcard;

        std::string to_string() const;
    };

    struct RestfulMapping {
        RestfulMethodPath path;
        std::string method_name;
    };

// Split components of `path_in' into `path_out'.
// * path_out->service_name does not have /.
// * path_out->prefix is normalized as
//   prefix := "/COMPONENT" prefix | "" (no dot in COMPONENT)
// Returns true on success.
    bool ParseRestfulPath(mutil::StringPiece path_in, RestfulMethodPath *path_out);

// Parse "PATH1 => NAME1, PATH2 => NAME2 ..." where:
// * PATHs are acceptible by ParseRestfulPath.
// * NAMEs are valid as method names in protobuf.
// Returns true on success.
    bool ParseRestfulMappings(const mutil::StringPiece &mappings,
                              std::vector<RestfulMapping> *list);

    struct RestfulMethodProperty : public Server::MethodProperty {
        RestfulMethodPath path;
        ServiceOwnership ownership;
    };

// Store paths under a same toplevel name.
    class RestfulMap {
    public:
        typedef std::map<std::string, RestfulMethodProperty> DedupMap;
        typedef std::vector<RestfulMethodProperty *> PathList;

        explicit RestfulMap(const std::string &service_name)
                : _service_name(service_name) {}

        virtual ~RestfulMap();

        // Map `path' to the method denoted by `method_name' in `service'.
        // Returns MethodStatus of the method on success, NULL otherwise.
        bool AddMethod(const RestfulMethodPath &path,
                       google::protobuf::Service *service,
                       const Server::MethodProperty::OpaqueParams &params,
                       const std::string &method_name,
                       MethodStatus *status);

        // Remove by RestfulMethodPath::to_string() of the path to AddMethod()
        // Returns number of methods removed (should be 1 or 0 currently)
        size_t RemoveByPathString(const std::string &path);

        // Remove all methods.
        void ClearMethods();

        // Called after by Server at starting moment, to refresh _sorted_paths
        void PrepareForFinding();

        // Find the method by path.
        // Time complexity in worst-case is #slashes-in-input * log(#paths-stored)
        const Server::MethodProperty *
        FindMethodProperty(const mutil::StringPiece &method_path,
                           std::string *unresolved_path) const;

        const std::string &service_name() const { return _service_name; }

        // Number of methods in this map. Only for UT right now.
        size_t size() const { return _dedup_map.size(); }

    private:
        DISALLOW_COPY_AND_ASSIGN(RestfulMap);

        std::string _service_name;
        // refreshed each time
        PathList _sorted_paths;
        DedupMap _dedup_map;
    };

    std::ostream &operator<<(std::ostream &os, const RestfulMethodPath &);

} // namespace melon


#endif  // MELON_RPC_RESTFUL_H_
