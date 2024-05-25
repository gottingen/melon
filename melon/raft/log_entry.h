// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//


#pragma once

#include <melon/utility/iobuf.h>                         // mutil::IOBuf
#include <melon/utility/memory/ref_counted.h>            // mutil::RefCountedThreadSafe
#include <melon/utility/third_party/murmurhash3/murmurhash3.h>  // fmix64
#include <melon/raft/configuration.h>
#include <melon/proto/raft/raft.pb.h>
#include <melon/raft/util.h>

namespace melon::raft {

// Log identifier
struct LogId {
    LogId() : index(0), term(0) {}
    LogId(int64_t index_, int64_t term_) : index(index_), term(term_) {}
    int64_t index;
    int64_t term;
};

// term start from 1, log index start from 1
struct LogEntry : public mutil::RefCountedThreadSafe<LogEntry> {
public:
    EntryType type; // log type
    LogId id;
    std::vector<PeerId>* peers; // peers
    std::vector<PeerId>* old_peers; // peers
    mutil::IOBuf data;

    LogEntry();

private:
    DISALLOW_COPY_AND_ASSIGN(LogEntry);
    friend class mutil::RefCountedThreadSafe<LogEntry>;
    virtual ~LogEntry();
};

// Comparators

inline bool operator==(const LogId& lhs, const LogId& rhs) {
    return lhs.index == rhs.index && lhs.term == rhs.term;
}

inline bool operator!=(const LogId& lhs, const LogId& rhs) {
    return !(lhs == rhs);
}

inline bool operator<(const LogId& lhs, const LogId& rhs) {
    if (lhs.term == rhs.term) {
        return lhs.index < rhs.index;
    }
    return lhs.term < rhs.term;
}

inline bool operator>(const LogId& lhs, const LogId& rhs) {
    if (lhs.term == rhs.term) {
        return lhs.index > rhs.index;
    }
    return lhs.term > rhs.term;
}

inline bool operator<=(const LogId& lhs, const LogId& rhs) {
    return !(lhs > rhs);
}

inline bool operator>=(const LogId& lhs, const LogId& rhs) {
    return !(lhs < rhs);
}

struct LogIdHasher {
    size_t operator()(const LogId& id) const {
        return mutil::fmix64(id.index) ^ mutil::fmix64(id.term);
    }
};

inline std::ostream& operator<<(std::ostream& os, const LogId& id) {
    os << "(index=" << id.index << ",term=" << id.term << ')';
    return os;
}

mutil::Status parse_configuration_meta(const mutil::IOBuf& data, LogEntry* entry);

mutil::Status serialize_configuration_meta(const LogEntry* entry, mutil::IOBuf& data);

}  //  namespace melon::raft
