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


// Date: Thu Oct 28 15:23:09 2010

#ifndef MUTIL_FILES_TEMP_FILE_H
#define MUTIL_FILES_TEMP_FILE_H

#include <melon/base/macros.h>

namespace mutil {

// Create a temporary file in current directory, which will be deleted when 
// corresponding TempFile object destructs, typically for unit testing.
// 
// Usage:
//   { 
//      TempFile tmpfile;           // A temporay file shall be created
//      tmpfile.save("some text");  // Write into the temporary file
//   }
//   // The temporary file shall be removed due to destruction of tmpfile
 
class TempFile {
public:
    // Create a temporary file in current directory. If |ext| is given,
    // filename will be temp_file_XXXXXX.|ext|, temp_file_XXXXXX otherwise.
    // If temporary file cannot be created, all save*() functions will
    // return -1. If |ext| is too long, filename will be truncated.
    TempFile();
    explicit TempFile(const char* ext);

    // The temporary file is removed in destructor.
    ~TempFile();

    // Save |content| to file, overwriting existing file.
    // Returns 0 when successful, -1 otherwise.
    int save(const char *content);

    // Save |fmt| and associated values to file, overwriting existing file.
    // Returns 0 when successful, -1 otherwise.
    int save_format(const char *fmt, ...) __attribute__((format (printf, 2, 3) ));

    // Save binary data |buf| (|count| bytes) to file, overwriting existing file.
    // Returns 0 when successful, -1 otherwise.
    int save_bin(const void *buf, size_t count);
    
    // Get name of the temporary file.
    const char *fname() const { return _fname; }

private:
    // TempFile is associated with file, copying makes no sense.
    DISALLOW_COPY_AND_ASSIGN(TempFile);
    
    int _reopen_if_necessary();
    
    int _fd;                // file descriptor
    int _ever_opened;
    char _fname[24];        // name of the file
};

} // namespace mutil

#endif  // MUTIL_FILES_TEMP_FILE_H
