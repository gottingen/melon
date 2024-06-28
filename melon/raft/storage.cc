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

#include <errno.h>
#include <melon/utility/string_printf.h>
#include <melon/utility/string_splitter.h>
#include <turbo/log/logging.h>
#include <melon/raft/storage.h>
#include <melon/raft/log.h>
#include <melon/raft/raft_meta.h>
#include <melon/raft/snapshot.h>

TURBO_FLAG(bool, raft_sync, true, "call fsync when need").on_validate(turbo::AllPassValidator<bool>::validate);
TURBO_FLAG(int32_t, raft_sync_per_bytes, INT32_MAX,
           "sync raft log per bytes when raft_sync set to true").on_validate(turbo::GeValidator<int32_t, 0>::validate);
TURBO_FLAG(bool, raft_create_parent_directories, true,
           "Create parent directories of the path in local storage if true");
TURBO_FLAG(int32_t, raft_sync_policy, 0,
           "raft sync policy when raft_sync set to true, 0 mean sync immediately, 1 mean sync by "
           "writed bytes");
TURBO_FLAG(bool, raft_sync_meta, false, "sync log meta, snapshot meta and raft meta").on_validate(
        turbo::AllPassValidator<bool>::validate);

namespace melon::raft {

    LogStorage *LogStorage::create(const std::string &uri) {
        mutil::StringPiece copied_uri(uri);
        std::string parameter;
        mutil::StringPiece protocol = parse_uri(&copied_uri, &parameter);
        if (protocol.empty()) {
            LOG(ERROR) << "Invalid log storage uri=`" << uri << '\'';
            return nullptr;
        }
        const LogStorage *type = log_storage_extension()->Find(
                protocol.as_string().c_str());
        if (type == nullptr) {
            LOG(ERROR) << "Fail to find log storage type " << protocol
                       << ", uri=" << uri;
            return nullptr;
        }
        return type->new_instance(parameter);
    }

    mutil::Status LogStorage::destroy(const std::string &uri) {
        mutil::Status status;
        mutil::StringPiece copied_uri(uri);
        std::string parameter;
        mutil::StringPiece protocol = parse_uri(&copied_uri, &parameter);
        if (protocol.empty()) {
            LOG(ERROR) << "Invalid log storage uri=`" << uri << '\'';
            status.set_error(EINVAL, "Invalid log storage uri = %s", uri.c_str());
            return status;
        }
        const LogStorage *type = log_storage_extension()->Find(
                protocol.as_string().c_str());
        if (type == nullptr) {
            LOG(ERROR) << "Fail to find log storage type " << protocol
                       << ", uri=" << uri;
            status.set_error(EINVAL, "Fail to find log storage type %s uri %s",
                             protocol.as_string().c_str(), uri.c_str());
            return status;
        }
        return type->gc_instance(parameter);
    }

    SnapshotStorage *SnapshotStorage::create(const std::string &uri) {
        mutil::StringPiece copied_uri(uri);
        std::string parameter;
        mutil::StringPiece protocol = parse_uri(&copied_uri, &parameter);
        if (protocol.empty()) {
            LOG(ERROR) << "Invalid snapshot storage uri=`" << uri << '\'';
            return nullptr;
        }
        const SnapshotStorage *type = snapshot_storage_extension()->Find(
                protocol.as_string().c_str());
        if (type == nullptr) {
            LOG(ERROR) << "Fail to find snapshot storage type " << protocol
                       << ", uri=" << uri;
            return nullptr;
        }
        return type->new_instance(parameter);
    }

    mutil::Status SnapshotStorage::destroy(const std::string &uri) {
        mutil::Status status;
        mutil::StringPiece copied_uri(uri);
        std::string parameter;
        mutil::StringPiece protocol = parse_uri(&copied_uri, &parameter);
        if (protocol.empty()) {
            LOG(ERROR) << "Invalid snapshot storage uri=`" << uri << '\'';
            status.set_error(EINVAL, "Invalid log storage uri = %s", uri.c_str());
            return status;
        }
        const SnapshotStorage *type = snapshot_storage_extension()->Find(
                protocol.as_string().c_str());
        if (type == nullptr) {
            LOG(ERROR) << "Fail to find snapshot storage type " << protocol
                       << ", uri=" << uri;
            status.set_error(EINVAL, "Fail to find snapshot storage type %s uri %s",
                             protocol.as_string().c_str(), uri.c_str());
            return status;
        }
        return type->gc_instance(parameter);
    }

    RaftMetaStorage *RaftMetaStorage::create(const std::string &uri) {
        mutil::StringPiece copied_uri(uri);
        std::string parameter;
        mutil::StringPiece protocol = parse_uri(&copied_uri, &parameter);
        if (protocol.empty()) {
            LOG(ERROR) << "Invalid meta storage uri=`" << uri << '\'';
            return nullptr;
        }
        const RaftMetaStorage *type = meta_storage_extension()->Find(
                protocol.as_string().c_str());
        if (type == nullptr) {
            LOG(ERROR) << "Fail to find meta storage type " << protocol
                       << ", uri=" << uri;
            return nullptr;
        }
        return type->new_instance(parameter);
    }

    mutil::Status RaftMetaStorage::destroy(const std::string &uri,
                                           const VersionedGroupId &vgid) {
        mutil::Status status;
        mutil::StringPiece copied_uri(uri);
        std::string parameter;
        mutil::StringPiece protocol = parse_uri(&copied_uri, &parameter);
        if (protocol.empty()) {
            LOG(ERROR) << "Invalid meta storage uri=`" << uri << '\'';
            status.set_error(EINVAL, "Invalid meta storage uri = %s", uri.c_str());
            return status;
        }
        const RaftMetaStorage *type = meta_storage_extension()->Find(
                protocol.as_string().c_str());
        if (type == nullptr) {
            LOG(ERROR) << "Fail to find meta storage type " << protocol
                       << ", uri=" << uri;
            status.set_error(EINVAL, "Fail to find meta storage type %s uri %s",
                             protocol.as_string().c_str(), uri.c_str());
            return status;
        }
        return type->gc_instance(parameter, vgid);
    }

}  //  namespace melon::raft
