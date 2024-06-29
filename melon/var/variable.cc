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


#include <pthread.h>
#include <set>                                  // std::set
#include <fstream>                              // std::ifstream
#include <sstream>                              // std::ostringstream
#include <turbo/flags/flag.h>
#include <melon/utility/macros.h>                        // MELON_CASSERT
#include <melon/utility/containers/flat_map.h>           // mutil::FlatMap
#include <melon/utility/scoped_lock.h>                   // MELON_SCOPE_LOCK
#include <melon/utility/string_splitter.h>               // mutil::StringSplitter
#include <melon/utility/errno.h>                         // berror
#include <melon/utility/time.h>                          // milliseconds_from_now
#include <melon/utility/file_util.h>                     // mutil::FilePath
#include <melon/utility/threading/platform_thread.h>
#include <melon/var/flag.h>
#include <melon/var/variable.h>
#include <melon/var/mvariable.h>
#include <turbo/flags/parse.h>

namespace melon::var {
    bool s_var_may_abort = false;

    static bool validate_var_abort_on_same_name(std::string_view value, std::string *error) noexcept {
        bool val;
        if (!turbo::parse_flag(value, &val, error)) {
            return false;
        }
        RELEASE_ASSERT_VERBOSE(!val || !s_var_may_abort, "Abort due to name conflict");
        return true;
    }

    static void enable_dumping_thread() noexcept;

    static void wakeup_dumping_thread() noexcept;
    static bool validate_mvar_dump_format(std::string_view value, std::string *err) noexcept;
}  // namespace melon::var
TURBO_FLAG(bool, save_series, true,
           "Save values of last 60 seconds, last 60 minutes,"
           " last 24 hours and last 30 days for plotting");

TURBO_FLAG(bool, quote_vector, true,
           "Quote description of Vector<> to make it valid to noah");

TURBO_FLAG(bool, var_abort_on_same_name, false,
           "Abort when names of var are same").on_validate(melon::var::validate_var_abort_on_same_name);
TURBO_FLAG(bool, var_log_dumpped, false,
           "[For debugging] print dumpped info"
           " into logstream before call Dumpper").on_validate(turbo::AllPassValidator<bool>::validate);

TURBO_FLAG(bool, var_dump,
           false,
           "Create a background thread dumping all var periodically, "
           "all var_dump_* flags are not effective when this flag is off").on_validate(
                turbo::AllPassValidator<bool>::validate)
        .on_update(melon::var::enable_dumping_thread);
TURBO_FLAG(int32_t, var_dump_interval,
           10, "Seconds between consecutive dump");
TURBO_FLAG(std::string, var_dump_file,
           "monitor/var.<app>.data", "Dump var into this file")
.on_validate(turbo::AllPassValidator<bool>::validate).on_update(melon::var::wakeup_dumping_thread);
TURBO_FLAG(std::string, var_dump_include,
           "", "Dump var matching these wildcards, "
               "separated by semicolon(;), empty means including all")
.on_validate(turbo::AllPassValidator<bool>::validate).on_update(melon::var::wakeup_dumping_thread);
TURBO_FLAG(std::string, var_dump_exclude,
           "", "Dump var excluded from these wildcards, "
               "separated by semicolon(;), empty means no exclusion")
.on_validate(turbo::AllPassValidator<bool>::validate).on_update(melon::var::wakeup_dumping_thread);
TURBO_FLAG(std::string, var_dump_prefix,
           "<app>", "Every dumped name starts with this prefix")
.on_validate(turbo::AllPassValidator<bool>::validate).on_update(melon::var::wakeup_dumping_thread);
TURBO_FLAG(std::string, var_dump_tabs,
           "latency=*_latency*"
           ";qps=*_qps*"
           ";error=*_error*"
           ";system=*process_*,*malloc_*,*kernel_*",
           "Dump var into different tabs according to the filters (separated by semicolon), "
           "format: *(tab_name=wildcards;)")
.on_validate(turbo::AllPassValidator<bool>::validate).on_update(melon::var::wakeup_dumping_thread);

TURBO_FLAG(bool, mvar_dump,
           false,
           "Create a background thread dumping(shares the same thread as var_dump) all mvar periodically, "
           "all mvar_dump_* flags are not effective when this flag is off").on_validate(
                turbo::AllPassValidator<bool>::validate)
        .on_update(melon::var::enable_dumping_thread);
TURBO_FLAG(std::string, mvar_dump_file,
           "monitor/mvar.<app>.data", "Dump mvar into this file")
.on_validate(turbo::AllPassValidator<bool>::validate).on_update(melon::var::wakeup_dumping_thread);
TURBO_FLAG(std::string, mvar_dump_prefix,
           "<app>", "Every dumped name starts with this prefix")
.on_validate(turbo::AllPassValidator<bool>::validate).on_update(melon::var::wakeup_dumping_thread);
TURBO_FLAG(std::string, mvar_dump_format,
           "common", "Dump mvar write format").on_validate(melon::var::validate_mvar_dump_format).on_update(
                melon::var::wakeup_dumping_thread);
namespace melon::var {


    const size_t SUB_MAP_COUNT = 32;  // must be power of 2
    MELON_CASSERT(!(SUB_MAP_COUNT & (SUB_MAP_COUNT - 1)), must_be_power_of_2);

    class VarEntry {
    public:
        VarEntry() : var(nullptr), display_filter(DISPLAY_ON_ALL) {}

        Variable *var;
        DisplayFilter display_filter;
    };

    typedef mutil::FlatMap<std::string, VarEntry> VarMap;

    struct VarMapWithLock : public VarMap {
        pthread_mutex_t mutex;

        VarMapWithLock() {
            CHECK_EQ(0, init(1024, 80));
            pthread_mutexattr_t attr;
            pthread_mutexattr_init(&attr);
            pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
            pthread_mutex_init(&mutex, &attr);
            pthread_mutexattr_destroy(&attr);
        }
    };

// We have to initialize global map on need because var is possibly used
// before main().
    static pthread_once_t s_var_maps_once = PTHREAD_ONCE_INIT;
    static VarMapWithLock *s_var_maps = nullptr;

    static void init_var_maps() {
        // It's probably slow to initialize all sub maps, but rpc often expose
        // variables before user. So this should not be an issue to users.
        s_var_maps = new VarMapWithLock[SUB_MAP_COUNT];
    }

    inline size_t sub_map_index(const std::string &str) {
        if (str.empty()) {
            return 0;
        }
        size_t h = 0;
        // we're assume that str is ended with '\0', which may not be in general
        for (const char *p = str.c_str(); *p; ++p) {
            h = h * 5 + *p;
        }
        return h & (SUB_MAP_COUNT - 1);
    }

    inline VarMapWithLock *get_var_maps() {
        pthread_once(&s_var_maps_once, init_var_maps);
        return s_var_maps;
    }

    inline VarMapWithLock &get_var_map(const std::string &name) {
        VarMapWithLock &m = get_var_maps()[sub_map_index(name)];
        return m;
    }

    Variable::~Variable() {
        CHECK(!hide()) << "Subclass of Variable MUST call hide() manually in their"
                          " dtors to avoid displaying a variable that is just destructing";
    }

    int Variable::expose_impl(const mutil::StringPiece &prefix,
                              const mutil::StringPiece &name,
                              DisplayFilter display_filter) {
        if (name.empty()) {
            LOG(ERROR) << "Parameter[name] is empty";
            return -1;
        }
        // NOTE: It's impossible to atomically erase from a submap and insert into
        // another submap without a global lock. When the to-be-exposed name
        // already exists, there's a chance that we can't insert back previous
        // name. But it should be fine generally because users are unlikely to
        // expose a variable more than once and calls to expose() are unlikely
        // to contend heavily.

        // remove previous pointer from the map if needed.
        hide();

        // Build the name.
        _name.clear();
        _name.reserve((prefix.size() + name.size()) * 5 / 4);
        if (!prefix.empty()) {
            to_underscored_name(&_name, prefix);
            if (!_name.empty() && mutil::back_char(_name) != '_') {
                _name.push_back('_');
            }
        }
        to_underscored_name(&_name, name);

        VarMapWithLock &m = get_var_map(_name);
        {
            MELON_SCOPED_LOCK(m.mutex);
            VarEntry *entry = m.seek(_name);
            if (entry == nullptr) {
                entry = &m[_name];
                entry->var = this;
                entry->display_filter = display_filter;
                return 0;
            }
        }
        RELEASE_ASSERT_VERBOSE(!turbo::get_flag(FLAGS_var_abort_on_same_name),
                               "Abort due to name conflict");
        if (!s_var_may_abort) {
            // Mark name conflict occurs, If this conflict happens before
            // initialization of var_abort_on_same_name, the validator will
            // abort the program if needed.
            s_var_may_abort = true;
        }

        LOG(ERROR) << "Already exposed `" << _name << "' whose value is `"
                   << describe_exposed(_name) << '\'';
        _name.clear();
        return -1;
    }

    bool Variable::is_hidden() const {
        return _name.empty();
    }

    bool Variable::hide() {
        if (_name.empty()) {
            return false;
        }
        VarMapWithLock &m = get_var_map(_name);
        MELON_SCOPED_LOCK(m.mutex);
        VarEntry *entry = m.seek(_name);
        if (entry) {
            CHECK_EQ(1UL, m.erase(_name));
        } else {
            CHECK(false) << "`" << _name << "' must exist";
        }
        _name.clear();
        return true;
    }

    void Variable::list_exposed(std::vector<std::string> *names,
                                DisplayFilter display_filter) {
        if (names == nullptr) {
            return;
        }
        names->clear();
        if (names->capacity() < 32) {
            names->reserve(count_exposed());
        }
        VarMapWithLock *var_maps = get_var_maps();
        for (size_t i = 0; i < SUB_MAP_COUNT; ++i) {
            VarMapWithLock &m = var_maps[i];
            std::unique_lock<pthread_mutex_t> mu(m.mutex);
            size_t n = 0;
            for (VarMap::const_iterator it = m.begin(); it != m.end(); ++it) {
                if (++n >= 256/*max iterated one pass*/) {
                    VarMap::PositionHint hint;
                    m.save_iterator(it, &hint);
                    n = 0;
                    mu.unlock();  // yield
                    mu.lock();
                    it = m.restore_iterator(hint);
                    if (it == m.begin()) { // resized
                        names->clear();
                    }
                    if (it == m.end()) {
                        break;
                    }
                }
                if (it->second.display_filter & display_filter) {
                    names->push_back(it->first);
                }
            }
        }
    }

    size_t Variable::count_exposed() {
        size_t n = 0;
        VarMapWithLock *var_maps = get_var_maps();
        for (size_t i = 0; i < SUB_MAP_COUNT; ++i) {
            n += var_maps[i].size();
        }
        return n;
    }

    int Variable::describe_exposed(const std::string &name, std::ostream &os,
                                   bool quote_string,
                                   DisplayFilter display_filter) {
        VarMapWithLock &m = get_var_map(name);
        MELON_SCOPED_LOCK(m.mutex);
        VarEntry *p = m.seek(name);
        if (p == nullptr) {
            return -1;
        }
        if (!(display_filter & p->display_filter)) {
            return -1;
        }
        p->var->describe(os, quote_string);
        return 0;
    }

    std::string Variable::describe_exposed(const std::string &name,
                                           bool quote_string,
                                           DisplayFilter display_filter) {
        std::ostringstream oss;
        if (describe_exposed(name, oss, quote_string, display_filter) == 0) {
            return oss.str();
        }
        return std::string();
    }

    std::string Variable::get_description() const {
        std::ostringstream os;
        describe(os, false);
        return os.str();
    }


    int Variable::describe_series_exposed(const std::string &name,
                                          std::ostream &os,
                                          const SeriesOptions &options) {
        VarMapWithLock &m = get_var_map(name);
        MELON_SCOPED_LOCK(m.mutex);
        VarEntry *p = m.seek(name);
        if (p == nullptr) {
            return -1;
        }
        return p->var->describe_series(os, options);
    }


// TODO(gejun): This is copied from otherwhere, common it if possible.

// Underlying buffer to store logs. Comparing to using std::ostringstream
// directly, this utility exposes more low-level methods so that we avoid
// creation of std::string which allocates memory internally.
    class CharArrayStreamBuf : public std::streambuf {
    public:
        explicit CharArrayStreamBuf() : _data(nullptr), _size(0) {}

        ~CharArrayStreamBuf();

        int overflow(int ch) override;

        int sync() override;

        void reset();

        mutil::StringPiece data() {
            return mutil::StringPiece(pbase(), pptr() - pbase());
        }

    private:
        char *_data;
        size_t _size;
    };

    CharArrayStreamBuf::~CharArrayStreamBuf() {
        free(_data);
    }

    int CharArrayStreamBuf::overflow(int ch) {
        if (ch == std::streambuf::traits_type::eof()) {
            return ch;
        }
        size_t new_size = std::max(_size * 3 / 2, (size_t) 64);
        char *new_data = (char *) malloc(new_size);
        if (MELON_UNLIKELY(new_data == nullptr)) {
            setp(nullptr, nullptr);
            return std::streambuf::traits_type::eof();
        }
        memcpy(new_data, _data, _size);
        free(_data);
        _data = new_data;
        const size_t old_size = _size;
        _size = new_size;
        setp(_data, _data + new_size);
        pbump(old_size);
        // if size == 1, this function will call overflow again.
        return sputc(ch);
    }

    int CharArrayStreamBuf::sync() {
        // data are already there.
        return 0;
    }

    void CharArrayStreamBuf::reset() {
        setp(_data, _data + _size);
    }


// Written by Jack Handy
// <A href="mailto:jakkhandy@hotmail.com">jakkhandy@hotmail.com</A>
    inline bool wildcmp(const char *wild, const char *str, char question_mark) {
        const char *cp = nullptr;
        const char *mp = nullptr;

        while (*str && *wild != '*') {
            if (*wild != *str && *wild != question_mark) {
                return false;
            }
            ++wild;
            ++str;
        }

        while (*str) {
            if (*wild == '*') {
                if (!*++wild) {
                    return true;
                }
                mp = wild;
                cp = str + 1;
            } else if (*wild == *str || *wild == question_mark) {
                ++wild;
                ++str;
            } else {
                wild = mp;
                str = cp++;
            }
        }

        while (*wild == '*') {
            ++wild;
        }
        return !*wild;
    }

    class WildcardMatcher {
    public:
        WildcardMatcher(const std::string &wildcards,
                        char question_mark,
                        bool on_both_empty)
                : _question_mark(question_mark), _on_both_empty(on_both_empty) {
            if (wildcards.empty()) {
                return;
            }
            std::string name;
            const char wc_pattern[3] = {'*', question_mark, '\0'};
            for (mutil::StringMultiSplitter sp(wildcards.c_str(), ",;");
                 sp != nullptr; ++sp) {
                name.assign(sp.field(), sp.length());
                if (name.find_first_of(wc_pattern) != std::string::npos) {
                    if (_wcs.empty()) {
                        _wcs.reserve(8);
                    }
                    _wcs.push_back(name);
                } else {
                    _exact.insert(name);
                }
            }
        }

        bool match(const std::string &name) const {
            if (!_exact.empty()) {
                if (_exact.find(name) != _exact.end()) {
                    return true;
                }
            } else if (_wcs.empty()) {
                return _on_both_empty;
            }
            for (size_t i = 0; i < _wcs.size(); ++i) {
                if (wildcmp(_wcs[i].c_str(), name.c_str(), _question_mark)) {
                    return true;
                }
            }
            return false;
        }

        const std::vector<std::string> &wildcards() const { return _wcs; }

        const std::set<std::string> &exact_names() const { return _exact; }

    private:
        char _question_mark;
        bool _on_both_empty;
        std::vector<std::string> _wcs;
        std::set<std::string> _exact;
    };

    DumpOptions::DumpOptions()
            : quote_string(true), question_mark('?'), display_filter(DISPLAY_ON_PLAIN_TEXT) {}

    int Variable::dump_exposed(Dumper *dumper, const DumpOptions *poptions) {
        if (nullptr == dumper) {
            LOG(ERROR) << "Parameter[dumper] is nullptr";
            return -1;
        }
        DumpOptions opt;
        if (poptions) {
            opt = *poptions;
        }
        CharArrayStreamBuf streambuf;
        std::ostream os(&streambuf);
        int count = 0;
        WildcardMatcher black_matcher(opt.black_wildcards,
                                      opt.question_mark,
                                      false);
        WildcardMatcher white_matcher(opt.white_wildcards,
                                      opt.question_mark,
                                      true);

        std::ostringstream dumpped_info;
        const bool log_dummped = turbo::get_flag(FLAGS_var_log_dumpped);

        if (white_matcher.wildcards().empty() &&
            !white_matcher.exact_names().empty()) {
            for (std::set<std::string>::const_iterator
                         it = white_matcher.exact_names().begin();
                 it != white_matcher.exact_names().end(); ++it) {
                const std::string &name = *it;
                if (!black_matcher.match(name)) {
                    if (melon::var::Variable::describe_exposed(
                            name, os, opt.quote_string, opt.display_filter) != 0) {
                        continue;
                    }
                    if (log_dummped) {
                        dumpped_info << '\n' << name << ": " << streambuf.data();
                    }
                    if (!dumper->dump(name, streambuf.data())) {
                        return -1;
                    }
                    streambuf.reset();
                    ++count;
                }
            }
        } else {
            // Have to iterate all variables.
            std::vector<std::string> varnames;
            melon::var::Variable::list_exposed(&varnames, opt.display_filter);
            // Sort the names to make them more readable.
            std::sort(varnames.begin(), varnames.end());
            for (std::vector<std::string>::const_iterator
                         it = varnames.begin(); it != varnames.end(); ++it) {
                const std::string &name = *it;
                if (white_matcher.match(name) && !black_matcher.match(name)) {
                    if (melon::var::Variable::describe_exposed(
                            name, os, opt.quote_string, opt.display_filter) != 0) {
                        continue;
                    }
                    if (log_dummped) {
                        dumpped_info << '\n' << name << ": " << streambuf.data();
                    }
                    if (!dumper->dump(name, streambuf.data())) {
                        return -1;
                    }
                    streambuf.reset();
                    ++count;
                }
            }
        }
        if (log_dummped) {
            LOG(INFO) << "Dumpped variables:" << dumpped_info.str();
        }
        return count;
    }


// ============= export to files ==============

    std::string read_command_name() {
        std::ifstream fin("/proc/self/stat");
        if (!fin.is_open()) {
            return std::string();
        }
        int pid = 0;
        std::string command_name;
        fin >> pid >> command_name;
        if (!fin.good()) {
            return std::string();
        }
        // Although the man page says the command name is in parenthesis, for
        // safety we normalize the name.
        std::string s;
        if (command_name.size() >= 2UL && command_name[0] == '(' &&
            mutil::back_char(command_name) == ')') {
            // remove parenthesis.
            to_underscored_name(&s,
                                mutil::StringPiece(command_name.data() + 1,
                                                   command_name.size() - 2UL));
        } else {
            to_underscored_name(&s, command_name);
        }
        return s;
    }

    class FileDumper : public Dumper {
    public:
        FileDumper(const std::string &filename, mutil::StringPiece s/*prefix*/)
                : _filename(filename), _fp(nullptr) {
            // setting prefix.
            // remove trailing spaces.
            const char *p = s.data() + s.size();
            for (; p != s.data() && isspace(p[-1]); --p) {}
            s.remove_suffix(s.data() + s.size() - p);
            // normalize it.
            if (!s.empty()) {
                to_underscored_name(&_prefix, s);
                if (mutil::back_char(_prefix) != '_') {
                    _prefix.push_back('_');
                }
            }
        }

        ~FileDumper() {
            close();
        }

        void close() {
            if (_fp) {
                fclose(_fp);
                _fp = nullptr;
            }
        }

    protected:
        bool dump_impl(const std::string &name, const mutil::StringPiece &desc, const std::string &separator) {
            if (_fp == nullptr) {
                mutil::File::Error error;
                mutil::FilePath dir = mutil::FilePath(_filename).DirName();
                if (!mutil::CreateDirectoryAndGetError(dir, &error)) {
                    LOG(ERROR) << "Fail to create directory=`" << dir.value()
                               << "', " << error;
                    return false;
                }
                _fp = fopen(_filename.c_str(), "w");
                if (nullptr == _fp) {
                    LOG(ERROR) << "Fail to open " << _filename;
                    return false;
                }
            }
            if (fprintf(_fp, "%.*s%.*s %.*s %.*s\r\n",
                        (int) _prefix.size(), _prefix.data(),
                        (int) name.size(), name.data(),
                        (int) separator.size(), separator.data(),
                        (int) desc.size(), desc.data()) < 0) {
                PLOG(ERROR) << "Fail to write into " << _filename;
                return false;
            }
            return true;
        }

    private:
        std::string _filename;
        FILE *_fp;
        std::string _prefix;
    };

    class CommonFileDumper : public FileDumper {
    public:
        CommonFileDumper(const std::string &filename, mutil::StringPiece prefix)
                : FileDumper(filename, prefix), _separator(":") {}

        bool dump(const std::string &name, const mutil::StringPiece &desc) {
            return dump_impl(name, desc, _separator);
        }

    private:
        std::string _separator;
    };

    class PrometheusFileDumper : public FileDumper {
    public:
        PrometheusFileDumper(const std::string &filename, mutil::StringPiece prefix)
                : FileDumper(filename, prefix), _separator(" ") {}

        bool dump(const std::string &name, const mutil::StringPiece &desc) {
            return dump_impl(name, desc, _separator);
        }

    private:
        std::string _separator;
    };

    class FileDumperGroup : public Dumper {
    public:
        FileDumperGroup(std::string tabs, std::string filename,
                        mutil::StringPiece s/*prefix*/) {
            mutil::FilePath path(filename);
            if (path.FinalExtension() == ".data") {
                // .data will be appended later
                path = path.RemoveFinalExtension();
            }

            for (mutil::KeyValuePairsSplitter sp(tabs, ';', '='); sp; ++sp) {
                std::string key = sp.key().as_string();
                std::string value = sp.value().as_string();
                FileDumper *f = new CommonFileDumper(
                        path.AddExtension(key).AddExtension("data").value(), s);
                WildcardMatcher *m = new WildcardMatcher(value, '?', true);
                dumpers.emplace_back(f, m);
            }
            dumpers.emplace_back(
                    new CommonFileDumper(path.AddExtension("data").value(), s),
                    (WildcardMatcher *) nullptr);
        }

        ~FileDumperGroup() {
            for (size_t i = 0; i < dumpers.size(); ++i) {
                delete dumpers[i].first;
                delete dumpers[i].second;
            }
            dumpers.clear();
        }

        bool dump(const std::string &name, const mutil::StringPiece &desc) override {
            for (size_t i = 0; i < dumpers.size() - 1; ++i) {
                if (dumpers[i].second->match(name)) {
                    return dumpers[i].first->dump(name, desc);
                }
            }
            // dump to default file
            return dumpers.back().first->dump(name, desc);
        }

    private:
        std::vector<std::pair<FileDumper *, WildcardMatcher *> > dumpers;
    };

    static pthread_once_t dumping_thread_once = PTHREAD_ONCE_INIT;
    static bool created_dumping_thread = false;
    static pthread_mutex_t dump_mutex = PTHREAD_MUTEX_INITIALIZER;
    static pthread_cond_t dump_cond = PTHREAD_COND_INITIALIZER;


#if !defined(UNIT_TEST)
    // Expose var-releated flags so that they're collected by noah.
    // Maybe useful when debugging process of monitoring.
    static Flag s_flag_var_dump_interval("var_dump_interval");
#endif

    // The background thread to export all var periodically.
    static void *dumping_thread(void *) {
        // NOTE: this variable was declared as static <= r34381, which was
        // destructed when program exits and caused coredumps.
        mutil::PlatformThread::SetName("var_dumper");
        const std::string command_name = read_command_name();
        std::string last_filename;
        std::string mvar_last_filename;
        while (1) {
            // We can't access string flags directly because it's thread-unsafe.
            std::string filename = turbo::get_flag(FLAGS_var_dump_file);
            DumpOptions options;
            std::string prefix = turbo::get_flag(FLAGS_var_dump_prefix);
            std::string tabs = turbo::get_flag(FLAGS_var_dump_tabs);
            std::string mvar_filename = turbo::get_flag(FLAGS_mvar_dump_file);
            std::string mvar_prefix = turbo::get_flag(FLAGS_mvar_dump_prefix);
            std::string mvar_format = turbo::get_flag(FLAGS_mvar_dump_format);
            options.white_wildcards = turbo::get_flag(FLAGS_var_dump_include);
            options.black_wildcards = turbo::get_flag(FLAGS_var_dump_exclude);

            if (turbo::get_flag(FLAGS_var_dump) && !filename.empty()) {
                // Replace first <app> in filename with program name. We can't use
                // pid because a same binary should write the data to the same
                // place, otherwise restarting of app may confuse noah with a lot
                // of *.data. noah takes 1.5 days to figure out that some data is
                // outdated and to be removed.
                const size_t pos = filename.find("<app>");
                if (pos != std::string::npos) {
                    filename.replace(pos, 5/*<app>*/, command_name);
                }
                if (last_filename != filename) {
                    last_filename = filename;
                    LOG(INFO) << "Write all var to " << filename << " every "
                              << turbo::get_flag(FLAGS_var_dump_interval) << " seconds.";
                }
                const size_t pos2 = prefix.find("<app>");
                if (pos2 != std::string::npos) {
                    prefix.replace(pos2, 5/*<app>*/, command_name);
                }
                FileDumperGroup dumper(tabs, filename, prefix);
                int nline = Variable::dump_exposed(&dumper, &options);
                if (nline < 0) {
                    LOG(ERROR) << "Fail to dump vars into " << filename;
                }
            }

            // Dump multi dimension var
            if (turbo::get_flag(FLAGS_mvar_dump) && !mvar_filename.empty()) {
                // Replace first <app> in filename with program name. We can't use
                // pid because a same binary should write the data to the same
                // place, otherwise restarting of app may confuse noah with a lot
                // of *.data. noah takes 1.5 days to figure out that some data is
                // outdated and to be removed.
                const size_t pos = mvar_filename.find("<app>");
                if (pos != std::string::npos) {
                    mvar_filename.replace(pos, 5/*<app>*/, command_name);
                }
                if (mvar_last_filename != mvar_filename) {
                    mvar_last_filename = mvar_filename;
                    LOG(INFO) << "Write all mvar to " << mvar_filename << " every "
                              << turbo::get_flag(FLAGS_var_dump_interval) << " seconds.";
                }
                const size_t pos2 = mvar_prefix.find("<app>");
                if (pos2 != std::string::npos) {
                    mvar_prefix.replace(pos2, 5/*<app>*/, command_name);
                }

                Dumper *dumper = nullptr;
                if ("common" == mvar_format) {
                    dumper = new CommonFileDumper(mvar_filename, mvar_prefix);
                } else if ("prometheus" == mvar_format) {
                    dumper = new PrometheusFileDumper(mvar_filename, mvar_prefix);
                }
                int nline = MVariable::dump_exposed(dumper, &options);
                if (nline < 0) {
                    LOG(ERROR) << "Fail to dump mvars into " << filename;
                }
                delete dumper;
                dumper = nullptr;
            }

            // We need to separate the sleeping into a long interruptible sleep
            // and a short uninterruptible sleep. Doing this because we wake up
            // this thread in gflag validators. If this thread dumps just after
            // waking up from the condition, the gflags may not even be updated.
            const int post_sleep_ms = 50;
            int cond_sleep_ms = turbo::get_flag(FLAGS_var_dump_interval) * 1000 - post_sleep_ms;
            if (cond_sleep_ms < 0) {
                LOG(ERROR) << "Bad cond_sleep_ms=" << cond_sleep_ms;
                cond_sleep_ms = 10000;
            }
            timespec deadline = mutil::milliseconds_from_now(cond_sleep_ms);
            pthread_mutex_lock(&dump_mutex);
            pthread_cond_timedwait(&dump_cond, &dump_mutex, &deadline);
            pthread_mutex_unlock(&dump_mutex);
            usleep(post_sleep_ms * 1000);
        }
    }

    static void launch_dumping_thread() {
        pthread_t thread_id;
        int rc = pthread_create(&thread_id, nullptr, dumping_thread, nullptr);
        if (rc != 0) {
            LOG(FATAL) << "Fail to launch dumping thread: " << berror(rc);
            return;
        }
        // Detach the thread because no one would join it.
        CHECK_EQ(0, pthread_detach(thread_id));
        created_dumping_thread = true;
    }

    // Start dumping_thread for only once.
    void enable_dumping_thread() noexcept {
        if (::turbo::get_flag(FLAGS_var_dump) || ::turbo::get_flag(FLAGS_mvar_dump))
            pthread_once(&dumping_thread_once, launch_dumping_thread);
    }

    void wakeup_dumping_thread() noexcept {
        // We're modifying a flag, wake up dumping_thread to generate
        // a new file soon.
        pthread_cond_signal(&dump_cond);
    }

    bool validate_mvar_dump_format(std::string_view value, std::string *err) noexcept {
        if (value != "common"
            && value != "prometheus") {
            LOG(ERROR) << "Invalid mvar_dump_format=" << value;
            if(err)
                *err = "Invalid mvar_dump_format";
            return false;
        }
        return true;
    }

    void to_underscored_name(std::string *name, const mutil::StringPiece &src) {
        name->reserve(name->size() + src.size() + 8/*just guess*/);
        for (const char *p = src.data(); p != src.data() + src.size(); ++p) {
            if (isalpha(*p)) {
                if (*p < 'a') { // upper cases
                    if (p != src.data() && !isupper(p[-1]) &&
                        mutil::back_char(*name) != '_') {
                        name->push_back('_');
                    }
                    name->push_back(*p - 'A' + 'a');
                } else {
                    name->push_back(*p);
                }
            } else if (isdigit(*p)) {
                name->push_back(*p);
            } else if (name->empty() || mutil::back_char(*name) != '_') {
                name->push_back('_');
            }
        }
    }

// UT don't need default variables.
#if !defined(UNIT_TEST)
    // Without these, default_variables.o are stripped.
    // At least working in gcc 4.8
    extern int do_link_default_variables;
    int dummy = do_link_default_variables;
#endif

}  // namespace melon::var
