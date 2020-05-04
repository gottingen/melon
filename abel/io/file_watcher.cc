
#include <sys/stat.h>
#include <abel/base/profile.h>
#include <abel/io/file_watcher.h>

namespace abel {

    file_watcher::file_watcher() : _last_ts() {
    }

    int file_watcher::init(const abel::filesystem::path &file_path) {
        if (init_from_not_exist(file_path) != 0) {
            return -1;
        }
        check_and_consume(NULL);
        return 0;
    }

    int file_watcher::init_from_not_exist(const abel::filesystem::path &file_path) {
        if (file_path.empty()) {
            return -1;
        }
        if (!_file_path.empty()) {
            return -1;
        }
        _file_path = file_path;
        return 0;
    }

    file_watcher::change file_watcher::check(abel::abel_time *new_timestamp) const {
        std::error_code ec;
        abel::filesystem::file_time_type st = abel::filesystem::last_write_time(_file_path, ec);
        if(ec) {
            *new_timestamp = abel::abel_time();
            if(_last_ts !=  abel::abel_time()) {
                return DELETED;
            } else {
                return UNCHANGED;
            }
        } else {
            *new_timestamp = abel::from_chrono(st);
            if(_last_ts != abel::abel_time()) {
                if(*new_timestamp != _last_ts) {
                    return UPDATED;
                } else {
                    return UNCHANGED;
                }
            } else {
                return CREATED;
            }
        }
    }

    file_watcher::change file_watcher::check_and_consume(abel::abel_time *last_timestamp) {
        abel::abel_time new_timestamp;
        change e = check(&new_timestamp);
        if (last_timestamp) {
            *last_timestamp = _last_ts;
        }
        if (e != UNCHANGED) {
            _last_ts = new_timestamp;
        }
        return e;
    }

    void file_watcher::restore(abel::abel_time timestamp) {
        _last_ts = timestamp;
    }

}  // namespace abel
