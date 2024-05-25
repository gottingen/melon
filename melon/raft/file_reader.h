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
#include <set>                              // std::set
#include <melon/utility/memory/ref_counted.h>        // mutil::RefCountedThreadsafe
#include <melon/utility/iobuf.h>                     // mutil::IOBuf
#include <melon/raft/macros.h>
#include <melon/raft/file_system_adaptor.h>

namespace melon::raft {

    // Abstract class to read data from a file
    // All the const method should be thread safe
    class FileReader : public mutil::RefCountedThreadSafe<FileReader> {
        friend class mutil::RefCountedThreadSafe<FileReader>;

    public:
        // Read data from filename at |offset| (from the start of the file) for at
        // most |max_count| bytes to |out|. Reading part of a file is allowed if
        // |read_partly| is TRUE. If successfully read the file, |read_count|
        // is the actual read count, it's maybe smaller than |max_count| if the
        // request is throttled or reach the end of the file. In the case of
        // reaching the end of the file, |is_eof| is also set to true.
        // Returns 0 on success, the error otherwise
        virtual int read_file(mutil::IOBuf *out,
                              const std::string &filename,
                              off_t offset,
                              size_t max_count,
                              bool read_partly,
                              size_t *read_count,
                              bool *is_eof) const = 0;

        // Get the path of this reader
        virtual const std::string &path() const = 0;

    protected:
        FileReader() {}

        virtual ~FileReader() {}
    };

    // Read files within a local directory
    class LocalDirReader : public FileReader {
    public:
        LocalDirReader(FileSystemAdaptor *fs, const std::string &path)
                : _path(path), _fs(fs), _current_file(NULL), _is_reading(false), _eof_reached(true) {}

        virtual ~LocalDirReader();

        // Open a snapshot for read
        virtual bool open();

        // Read data from filename at |offset| (from the start of the file) for at
        // most |max_count| bytes to |out|. Reading part of a file is allowed if
        // |read_partly| is TRUE. If successfully read the file, |read_count|
        // is the actual read count, it's maybe smaller than |max_count| if the
        // request is throttled or reach the end of the file. In the case of
        // reaching the end of the file, |is_eof| is also set to true.
        // Returns 0 on success, the error otherwise
        virtual int read_file(mutil::IOBuf *out,
                              const std::string &filename,
                              off_t offset,
                              size_t max_count,
                              bool read_partly,
                              size_t *read_count,
                              bool *is_eof) const;

        virtual const std::string &path() const { return _path; }

    protected:
        int read_file_with_meta(mutil::IOBuf *out,
                                const std::string &filename,
                                google::protobuf::Message *file_meta,
                                off_t offset,
                                size_t max_count,
                                size_t *read_count,
                                bool *is_eof) const;

        const scoped_refptr<FileSystemAdaptor> &file_system() const { return _fs; }

    private:
        mutable raft_mutex_t _mutex;
        std::string _path;
        scoped_refptr<FileSystemAdaptor> _fs;
        mutable FileAdaptor *_current_file;
        mutable std::string _current_filename;
        mutable bool _is_reading;
        mutable bool _eof_reached;
    };

}  //  namespace melon::raft
