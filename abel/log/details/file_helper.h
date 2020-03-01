//
// Copyright(c) 2015 Gabi Melman.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#pragma once

#include <abel/log/details/log_msg.h>
#include <abel/filesystem/filesystem.h>
#include <abel/filesystem/filesystem.h>
#include <abel/chrono/clock.h>
#include <abel/system/fd_util.h>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <string>
#include <thread>
#include <tuple>

namespace abel {
    namespace log {
        namespace details {

            static const char *default_eol = ABEL_EOL;

            inline bool fopen_s(FILE **fp, const filename_t &filename, const filename_t &mode) {
#ifdef _WIN32
#ifndef ABEL_WCHAR_T_NON_NATIVE
                *fp = _wfsopen((filename.c_str()), mode.c_str(), _SH_DENYWR);
#else
                *fp = _fsopen((filename.c_str()), mode.c_str(), _SH_DENYWR);
#endif
#else // unix
                *fp = fopen((filename.c_str()), mode.c_str());
#endif

                if (*fp != nullptr) {
                    abel::prevent_child_fd(*fp);
                }

                return *fp == nullptr;
            }

#if defined(_WIN32) && !defined(ABEL_WCHAR_T_NON_NATIVE)
#define ABEL_LOG_FILENAME_T(s) L##s
            inline std::string filename_to_str(const filename_t &filename)
            {
                std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> c;
                return c.to_bytes(filename);
            }
#else
#define ABEL_LOG_FILENAME_T(s) s

            inline std::string filename_to_str(const filename_t &filename) {
                return filename;
            }

#endif

            class file_helper {

            public:
                const int open_tries = 5;
                const int open_interval = 10;

                explicit file_helper() = default;

                file_helper(const file_helper &) = delete;

                file_helper &operator=(const file_helper &) = delete;

                ~file_helper() {
                    close();
                }

                void open(const filename_t &fname, bool truncate = false) {
                    close();
                    auto *mode = truncate ? ABEL_LOG_FILENAME_T("wb") : ABEL_LOG_FILENAME_T("ab");
                    _filename = fname;
                    for (int tries = 0; tries < open_tries; ++tries) {
                        if (!fopen_s(&fd_, fname, mode)) {
                            return;
                        }
                        abel::sleep_for(abel::milliseconds(open_interval));
                    }

                    throw log_ex("Failed opening file " + filename_to_str(_filename) + " for writing", errno);
                }

                void reopen(bool truncate) {
                    if (_filename.empty()) {
                        throw log_ex("Failed re opening file - was not opened before");
                    }
                    open(_filename, truncate);
                }

                void flush() {
                    std::fflush(fd_);
                }

                void close() {
                    if (fd_ != nullptr) {
                        std::fclose(fd_);
                        fd_ = nullptr;
                    }
                }

                void write(const fmt::memory_buffer &buf) {
                    size_t msg_size = buf.size();
                    auto data = buf.data();
                    if (std::fwrite(data, 1, msg_size, fd_) != msg_size) {
                        throw log_ex("Failed writing to file " + filename_to_str(_filename), errno);
                    }
                }

                size_t size() const {
                    if (fd_ == nullptr) {
                        throw log_ex("Cannot use size() on closed file " + filename_to_str(_filename));
                    }
                    return abel::filesystem::file_size(_filename);
                }

                const filename_t &filename() const {
                    return _filename;
                }

                static bool file_exists(const filename_t &fname) {
                    return abel::filesystem::exists(fname);
                }

                //
                // return file path and its extension:
                //
                // "mylog.txt" => ("mylog", ".txt")
                // "mylog" => ("mylog", "")
                // "mylog." => ("mylog.", "")
                // "/dir1/dir2/mylog.txt" => ("/dir1/dir2/mylog", ".txt")
                //
                // the starting dot in filenames is ignored (hidden files):
                //
                // ".mylog" => (".mylog". "")
                // "my_folder/.mylog" => ("my_folder/.mylog", "")
                // "my_folder/.mylog.txt" => ("my_folder/.mylog", ".txt")
                static std::tuple<filename_t, filename_t> split_by_extenstion(const abel::log::filename_t &fname) {
                    auto ext_index = fname.rfind('.');

                    // no valid extension found - return whole path and empty string as
                    // extension
                    if (ext_index == filename_t::npos || ext_index == 0 || ext_index == fname.size() - 1) {
                        return std::make_tuple(fname, abel::log::filename_t());
                    }

                    // treat casese like "/etc/rc.d/somelogfile or "/abc/.hiddenfile"
                    auto folder_index = fname.rfind(abel::filesystem::path::preferred_separator);
                    if (folder_index != fname.npos && folder_index >= ext_index - 1) {
                        return std::make_tuple(fname, abel::log::filename_t());
                    }

                    // finally - return a valid base and extension tuple
                    return std::make_tuple(fname.substr(0, ext_index), fname.substr(ext_index));
                }

            private:
                FILE *fd_{nullptr};
                filename_t _filename;
            };
        } // namespace details
    } //namespace log
} // namespace abel
