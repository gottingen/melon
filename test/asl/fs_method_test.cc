//
// Created by liyinbin on 2020/1/27.
//

#include <test/testing/filesystem_test_util.h>
#include <abel/chrono/time.h>
#include <gtest/gtest.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <locale>
#include <iomanip>

TEST(fs, canonical) {
    EXPECT_THROW(fs::canonical(""), fs::filesystem_error);
    {
        std::error_code ec;
        EXPECT_TRUE(fs::canonical("", ec) == "");
        EXPECT_TRUE(ec);
    }
    EXPECT_TRUE(fs::canonical(fs::current_path()) == fs::current_path());

    EXPECT_TRUE(fs::canonical(".") == fs::current_path());
    EXPECT_TRUE(fs::canonical("..") == fs::current_path().parent_path());
    EXPECT_TRUE(fs::canonical("/") == fs::current_path().root_path());
    EXPECT_THROW(fs::canonical("foo"), fs::filesystem_error);
    {
        std::error_code ec;
        EXPECT_NO_THROW(fs::canonical("foo", ec));
        EXPECT_TRUE(ec);
    }
    {
        TemporaryDirectory t(TempOpt::change_path);
        auto dir = t.path() / "d0";
        fs::create_directories(dir / "d1");
        generateFile(dir / "f0");
        fs::path rel(dir.filename());
        EXPECT_TRUE(fs::canonical(dir) == dir);
        EXPECT_TRUE(fs::canonical(rel) == dir);
        EXPECT_TRUE(fs::canonical(dir / "f0") == dir / "f0");
        EXPECT_TRUE(fs::canonical(rel / "f0") == dir / "f0");
        EXPECT_TRUE(fs::canonical(rel / "./f0") == dir / "f0");
        EXPECT_TRUE(fs::canonical(rel / "d1/../f0") == dir / "f0");
    }

    if (is_symlink_creation_supported()) {
        TemporaryDirectory t(TempOpt::change_path);
        fs::create_directory(t.path() / "dir1");
        generateFile(t.path() / "dir1/test1");
        fs::create_directory(t.path() / "dir2");
        fs::create_directory_symlink(t.path() / "dir1", t.path() / "dir2/dirSym");
        EXPECT_TRUE(fs::canonical(t.path() / "dir2/dirSym/test1") == t.path() / "dir1/test1");
    }
}

TEST(fs, copy) {
    {
        TemporaryDirectory t(TempOpt::change_path);
        std::error_code ec;
        fs::create_directory("dir1");
        generateFile("dir1/file1");
        generateFile("dir1/file2");
        fs::create_directory("dir1/dir2");
        generateFile("dir1/dir2/file3");
        EXPECT_NO_THROW(fs::copy("dir1", "dir3"));
        EXPECT_TRUE(fs::exists("dir3/file1"));
        EXPECT_TRUE(fs::exists("dir3/file2"));
        EXPECT_TRUE(!fs::exists("dir3/dir2"));
        EXPECT_NO_THROW(fs::copy("dir1", "dir4", fs::copy_options::recursive, ec));
        EXPECT_TRUE(!ec);
        EXPECT_TRUE(fs::exists("dir4/file1"));
        EXPECT_TRUE(fs::exists("dir4/file2"));
        EXPECT_TRUE(fs::exists("dir4/dir2/file3"));
        fs::create_directory("dir5");
        generateFile("dir5/file1");
        EXPECT_THROW(fs::copy("dir1/file1", "dir5/file1"), fs::filesystem_error);
        EXPECT_NO_THROW(fs::copy("dir1/file1", "dir5/file1", fs::copy_options::skip_existing));
    }
    if (is_symlink_creation_supported()) {
        TemporaryDirectory t(TempOpt::change_path);
        std::error_code ec;
        fs::create_directory("dir1");
        generateFile("dir1/file1");
        generateFile("dir1/file2");
        fs::create_directory("dir1/dir2");
        generateFile("dir1/dir2/file3");
#ifdef TEST_LWG_2682_BEHAVIOUR
        EXPECT_THROW(fs::copy("dir1", "dir3", fs::copy_options::create_symlinks | fs::copy_options::recursive),
                     fs::filesystem_error);
#else
        EXPECT_NO_THROW(fs::copy("dir1", "dir3", fs::copy_options::create_symlinks | fs::copy_options::recursive));
                EXPECT_TRUE(!ec);
                EXPECT_TRUE(fs::exists("dir3/file1"));
                EXPECT_TRUE(fs::is_symlink("dir3/file1"));
                EXPECT_TRUE(fs::exists("dir3/file2"));
                EXPECT_TRUE(fs::is_symlink("dir3/file2"));
                EXPECT_TRUE(fs::exists("dir3/dir2/file3"));
                EXPECT_TRUE(fs::is_symlink("dir3/dir2/file3"));
#endif
    }
    {
        TemporaryDirectory t(TempOpt::change_path);
        std::error_code ec;
        fs::create_directory("dir1");
        generateFile("dir1/file1");
        generateFile("dir1/file2");
        fs::create_directory("dir1/dir2");
        generateFile("dir1/dir2/file3");
        auto f1hl = fs::hard_link_count("dir1/file1");
        auto f2hl = fs::hard_link_count("dir1/file2");
        auto f3hl = fs::hard_link_count("dir1/dir2/file3");
        EXPECT_NO_THROW(fs::copy("dir1",
                                 "dir3",
                                 fs::copy_options::create_hard_links | fs::copy_options::recursive,
                                 ec));
        EXPECT_TRUE(!ec);
        EXPECT_TRUE(fs::exists("dir3/file1"));
        EXPECT_TRUE(fs::hard_link_count("dir1/file1") == f1hl + 1);
        EXPECT_TRUE(fs::exists("dir3/file2"));
        EXPECT_TRUE(fs::hard_link_count("dir1/file2") == f2hl + 1);
        EXPECT_TRUE(fs::exists("dir3/dir2/file3"));
        EXPECT_TRUE(fs::hard_link_count("dir1/dir2/file3") == f3hl + 1);
    }
}

TEST(fs, copy_file) {
    TemporaryDirectory t(TempOpt::change_path);
    std::error_code ec;
    generateFile("foo", 100);
    EXPECT_TRUE(!fs::exists("bar"));
    EXPECT_TRUE(fs::copy_file("foo", "bar"));
    EXPECT_TRUE(fs::exists("bar"));
    EXPECT_TRUE(fs::file_size("foo") == fs::file_size("bar"));
    EXPECT_TRUE(fs::copy_file("foo", "bar2", ec));
    EXPECT_TRUE(!ec);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    generateFile("foo2", 200);
    EXPECT_TRUE(fs::copy_file("foo2", "bar", fs::copy_options::update_existing));
    EXPECT_TRUE(fs::file_size("bar") == 200);
    EXPECT_TRUE(!fs::copy_file("foo", "bar", fs::copy_options::update_existing));
    EXPECT_TRUE(fs::file_size("bar") == 200);
    EXPECT_TRUE(fs::copy_file("foo", "bar", fs::copy_options::overwrite_existing));
    EXPECT_TRUE(fs::file_size("bar") == 100);
    EXPECT_THROW(fs::copy_file("foobar", "foobar2"), fs::filesystem_error);
    EXPECT_NO_THROW(fs::copy_file("foobar", "foobar2", ec));
    EXPECT_TRUE(ec);
    EXPECT_TRUE(!fs::exists("foobar"));
}

TEST(fs, copy_symlink) {
    TemporaryDirectory t(TempOpt::change_path);
    std::error_code ec;
    generateFile("foo");
    fs::create_directory("dir");
    if (is_symlink_creation_supported()) {
        fs::create_symlink("foo", "sfoo");
        fs::create_directory_symlink("dir", "sdir");
        EXPECT_NO_THROW(fs::copy_symlink("sfoo", "sfooc"));
        EXPECT_TRUE(fs::exists("sfooc"));
        EXPECT_NO_THROW(fs::copy_symlink("sfoo", "sfooc2", ec));
        EXPECT_TRUE(fs::exists("sfooc2"));
        EXPECT_TRUE(!ec);
        EXPECT_NO_THROW(fs::copy_symlink("sdir", "sdirc"));
        EXPECT_TRUE(fs::exists("sdirc"));
        EXPECT_NO_THROW(fs::copy_symlink("sdir", "sdirc2", ec));
        EXPECT_TRUE(fs::exists("sdirc2"));
        EXPECT_TRUE(!ec);
    }
    EXPECT_THROW(fs::copy_symlink("bar", "barc"), fs::filesystem_error);
    EXPECT_NO_THROW(fs::copy_symlink("bar", "barc", ec));
    EXPECT_TRUE(ec);
}

TEST(fs, create_directories) {
    TemporaryDirectory t;
    fs::path p = t.path() / "testdir";
    fs::path p2 = p / "nested";
    EXPECT_TRUE(!fs::exists(p));
    EXPECT_TRUE(!fs::exists(p2));
    EXPECT_TRUE(fs::create_directories(p2));
    EXPECT_TRUE(fs::is_directory(p));
    EXPECT_TRUE(fs::is_directory(p2));
#ifdef TEST_LWG_2935_BEHAVIOUR
    EXPECT_TRUE("This test expects LWG #2935 result conformance.");
        p = t.path() / "testfile";
        generateFile(p);
        EXPECT_TRUE(fs::is_regular_file(p));
        EXPECT_TRUE(!fs::is_directory(p));
        bool created = false;
        EXPECT_NO_THROW((created = fs::create_directories(p)));
        EXPECT_TRUE(!created);
        EXPECT_TRUE(fs::is_regular_file(p));
        EXPECT_TRUE(!fs::is_directory(p));
        std::error_code ec;
        EXPECT_NO_THROW((created = fs::create_directories(p, ec)));
        EXPECT_TRUE(!created);
        EXPECT_TRUE(!ec);
        EXPECT_TRUE(fs::is_regular_file(p));
        EXPECT_TRUE(!fs::is_directory(p));
        EXPECT_TRUE(!fs::create_directories(p, ec));
#else
    EXPECT_TRUE("This test expects conformance with P1164R1. (implemented by GCC with issue #86910.)");
    p = t.path() / "testfile";
    generateFile(p);
    EXPECT_TRUE(fs::is_regular_file(p));
    EXPECT_TRUE(!fs::is_directory(p));
    EXPECT_THROW(fs::create_directories(p), fs::filesystem_error);
    EXPECT_TRUE(fs::is_regular_file(p));
    EXPECT_TRUE(!fs::is_directory(p));
    std::error_code ec;
    EXPECT_NO_THROW(fs::create_directories(p, ec));
    EXPECT_TRUE(ec);
    EXPECT_TRUE(fs::is_regular_file(p));
    EXPECT_TRUE(!fs::is_directory(p));
    EXPECT_TRUE(!fs::create_directories(p, ec));
#endif
}

TEST(fs, create_directory) {
    TemporaryDirectory t;
    fs::path p = t.path() / "testdir";
    EXPECT_TRUE(!fs::exists(p));
    EXPECT_TRUE(fs::create_directory(p));
    EXPECT_TRUE(fs::is_directory(p));
    EXPECT_TRUE(!fs::is_regular_file(p));
    EXPECT_TRUE(fs::create_directory(p / "nested", p));
    EXPECT_TRUE(fs::is_directory(p / "nested"));
    EXPECT_TRUE(!fs::is_regular_file(p / "nested"));
#ifdef TEST_LWG_2935_BEHAVIOUR
    EXPECT_TRUE("This test expects LWG #2935 result conformance.");
        p = t.path() / "testfile";
        generateFile(p);
        EXPECT_TRUE(fs::is_regular_file(p));
        EXPECT_TRUE(!fs::is_directory(p));
        bool created = false;
        EXPECT_NO_THROW((created = fs::create_directory(p)));
        EXPECT_TRUE(!created);
        EXPECT_TRUE(fs::is_regular_file(p));
        EXPECT_TRUE(!fs::is_directory(p));
        std::error_code ec;
        EXPECT_NO_THROW((created = fs::create_directory(p, ec)));
        EXPECT_TRUE(!created);
        EXPECT_TRUE(!ec);
        EXPECT_TRUE(fs::is_regular_file(p));
        EXPECT_TRUE(!fs::is_directory(p));
        EXPECT_TRUE(!fs::create_directories(p, ec));
#else
    EXPECT_TRUE("This test expects conformance with P1164R1. (implemented by GCC with issue #86910.)");
    p = t.path() / "testfile";
    generateFile(p);
    EXPECT_TRUE(fs::is_regular_file(p));
    EXPECT_TRUE(!fs::is_directory(p));
    EXPECT_THROW(fs::create_directory(p), fs::filesystem_error);
    EXPECT_TRUE(fs::is_regular_file(p));
    EXPECT_TRUE(!fs::is_directory(p));
    std::error_code ec;
    EXPECT_NO_THROW(fs::create_directory(p, ec));
    EXPECT_TRUE(ec);
    EXPECT_TRUE(fs::is_regular_file(p));
    EXPECT_TRUE(!fs::is_directory(p));
    EXPECT_TRUE(!fs::create_directory(p, ec));
#endif
}

TEST(fs, create_directory_symlink) {
    if (is_symlink_creation_supported()) {
        TemporaryDirectory t;
        fs::create_directory(t.path() / "dir1");
        generateFile(t.path() / "dir1/test1");
        fs::create_directory(t.path() / "dir2");
        fs::create_directory_symlink(t.path() / "dir1", t.path() / "dir2/dirSym");
        EXPECT_TRUE(fs::exists(t.path() / "dir2/dirSym"));
        EXPECT_TRUE(fs::is_symlink(t.path() / "dir2/dirSym"));
        EXPECT_TRUE(fs::exists(t.path() / "dir2/dirSym/test1"));
        EXPECT_TRUE(fs::is_regular_file(t.path() / "dir2/dirSym/test1"));
        EXPECT_THROW(fs::create_directory_symlink(t.path() / "dir1", t.path() / "dir2/dirSym"), fs::filesystem_error);
        std::error_code ec;
        EXPECT_NO_THROW(fs::create_directory_symlink(t.path() / "dir1", t.path() / "dir2/dirSym", ec));
        EXPECT_TRUE(ec);
    }
}

TEST(fs, create_hard_link) {
    TemporaryDirectory t(TempOpt::change_path);
    std::error_code ec;
    generateFile("foo", 1234);
    EXPECT_NO_THROW(fs::create_hard_link("foo", "bar"));
    EXPECT_TRUE(fs::exists("bar"));
    EXPECT_TRUE(!fs::is_symlink("bar"));
    EXPECT_NO_THROW(fs::create_hard_link("foo", "bar2", ec));
    EXPECT_TRUE(fs::exists("bar2"));
    EXPECT_TRUE(!fs::is_symlink("bar2"));
    EXPECT_TRUE(!ec);
    EXPECT_THROW(fs::create_hard_link("nofoo", "bar"), fs::filesystem_error);
    EXPECT_NO_THROW(fs::create_hard_link("nofoo", "bar", ec));
    EXPECT_TRUE(ec);
}

TEST(fs, create_symlink) {
    if (is_symlink_creation_supported()) {
        TemporaryDirectory t;
        fs::create_directory(t.path() / "dir1");
        generateFile(t.path() / "dir1/test1");
        fs::create_directory(t.path() / "dir2");
        fs::create_symlink(t.path() / "dir1/test1", t.path() / "dir2/fileSym");
        EXPECT_TRUE(fs::exists(t.path() / "dir2/fileSym"));
        EXPECT_TRUE(fs::is_symlink(t.path() / "dir2/fileSym"));
        EXPECT_TRUE(fs::exists(t.path() / "dir2/fileSym"));
        EXPECT_TRUE(fs::is_regular_file(t.path() / "dir2/fileSym"));
        EXPECT_THROW(fs::create_symlink(t.path() / "dir1", t.path() / "dir2/fileSym"), fs::filesystem_error);
        std::error_code ec;
        EXPECT_NO_THROW(fs::create_symlink(t.path() / "dir1", t.path() / "dir2/fileSym", ec));
        EXPECT_TRUE(ec);
    }
}

TEST(fs, current_path) {
    TemporaryDirectory t;
    std::error_code ec;
    fs::path p1 = fs::current_path();
    EXPECT_NO_THROW(fs::current_path(t.path()));
    EXPECT_TRUE(p1 != fs::current_path());
    EXPECT_NO_THROW(fs::current_path(p1, ec));
    EXPECT_TRUE(!ec);
    EXPECT_THROW(fs::current_path(t.path() / "foo"), fs::filesystem_error);
    EXPECT_TRUE(p1 == fs::current_path());
    EXPECT_NO_THROW(fs::current_path(t.path() / "foo", ec));
    EXPECT_TRUE(ec);
}

TEST(fs, equivalent) {
    TemporaryDirectory t(TempOpt::change_path);
    generateFile("foo", 1234);
    EXPECT_TRUE(fs::equivalent(t.path() / "foo", "foo"));
    if (is_symlink_creation_supported()) {
        std::error_code ec(42, std::system_category());
        fs::create_symlink("foo", "foo2");
        EXPECT_TRUE(fs::equivalent("foo", "foo2"));
        EXPECT_TRUE(fs::equivalent("foo", "foo2", ec));
        EXPECT_TRUE(!ec);
    }
#ifdef TEST_LWG_2937_BEHAVIOUR
    EXPECT_TRUE("This test expects LWG #2937 result conformance.");
    std::error_code ec;
    bool result = false;
    EXPECT_THROW(fs::equivalent("foo", "foo3"), fs::filesystem_error);
    EXPECT_NO_THROW(result = fs::equivalent("foo", "foo3", ec));
    EXPECT_TRUE(!result);
    EXPECT_TRUE(ec);
    ec.clear();
    EXPECT_THROW(fs::equivalent("foo3", "foo"), fs::filesystem_error);
    EXPECT_NO_THROW(result = fs::equivalent("foo3", "foo", ec));
    EXPECT_TRUE(!result);
    EXPECT_TRUE(ec);
    ec.clear();
    EXPECT_THROW(fs::equivalent("foo3", "foo4"), fs::filesystem_error);
    EXPECT_NO_THROW(result = fs::equivalent("foo3", "foo4", ec));
    EXPECT_TRUE(!result);
    EXPECT_TRUE(ec);
#else
    EXPECT_TRUE("This test expects conformance predating LWG #2937 result.");
        std::error_code ec;
        bool result = false;
        EXPECT_NO_THROW(result = fs::equivalent("foo", "foo3"));
        EXPECT_TRUE(!result);
        EXPECT_NO_THROW(result = fs::equivalent("foo", "foo3", ec));
        EXPECT_TRUE(!result);
        EXPECT_TRUE(!ec);
        ec.clear();
        EXPECT_NO_THROW(result = fs::equivalent("foo3", "foo"));
        EXPECT_TRUE(!result);
        EXPECT_NO_THROW(result = fs::equivalent("foo3", "foo", ec));
        EXPECT_TRUE(!result);
        EXPECT_TRUE(!ec);
        ec.clear();
        EXPECT_THROW(result = fs::equivalent("foo4", "foo3"), fs::filesystem_error);
        EXPECT_TRUE(!result);
        EXPECT_NO_THROW(result = fs::equivalent("foo4", "foo3", ec));
        EXPECT_TRUE(!result);
        EXPECT_TRUE(ec);
#endif
}

TEST(fs, exists) {
    TemporaryDirectory t(TempOpt::change_path);
    std::error_code ec;
    EXPECT_TRUE(!fs::exists(""));
    EXPECT_TRUE(!fs::exists("foo"));
    EXPECT_TRUE(!fs::exists("foo", ec));
    EXPECT_TRUE(!ec);
    ec = std::error_code(42, std::system_category());
    EXPECT_TRUE(!fs::exists("foo", ec));
    EXPECT_TRUE(!ec);
    ec.clear();
    EXPECT_TRUE(fs::exists(t.path()));
    EXPECT_TRUE(fs::exists(t.path(), ec));
    EXPECT_TRUE(!ec);
    ec = std::error_code(42, std::system_category());
    EXPECT_TRUE(fs::exists(t.path(), ec));
    EXPECT_TRUE(!ec);
}

TEST(fs, file_size) {
    TemporaryDirectory t(TempOpt::change_path);
    std::error_code ec;
    generateFile("foo", 0);
    generateFile("bar", 1234);
    EXPECT_TRUE(fs::file_size("foo") == 0);
    ec = std::error_code(42, std::system_category());
    EXPECT_TRUE(fs::file_size("foo", ec) == 0);
    EXPECT_TRUE(!ec);
    ec.clear();
    EXPECT_TRUE(fs::file_size("bar") == 1234);
    ec = std::error_code(42, std::system_category());
    EXPECT_TRUE(fs::file_size("bar", ec) == 1234);
    EXPECT_TRUE(!ec);
    ec.clear();
    EXPECT_THROW(fs::file_size("foobar"), fs::filesystem_error);
    EXPECT_TRUE(fs::file_size("foobar", ec) == static_cast<uintmax_t>(-1));
    EXPECT_TRUE(ec);
    ec.clear();
}

TEST(fs, hard_link_count) {
    TemporaryDirectory t(TempOpt::change_path);
    std::error_code ec;
#ifdef ABEL_PLATFORM_WINDOWS
    // windows doesn't implement "."/".." as hardlinks, so it
        // starts with 1 and subdirectories don't change the count
        EXPECT_TRUE(fs::hard_link_count(t.path()) == 1);
        fs::create_directory("dir");
        EXPECT_TRUE(fs::hard_link_count(t.path()) == 1);
#else
// unix/bsd/linux typically implements "."/".." as hardlinks
// so an empty dir has 2 (from parent and the ".") and
// adding a subdirectory adds one due to its ".."
    EXPECT_TRUE(fs::hard_link_count(t.path()) == 2);
    fs::create_directory("dir");
    EXPECT_TRUE(fs::hard_link_count(t.path()) == 3);
#endif
    generateFile("foo");
    EXPECT_TRUE(fs::hard_link_count(t.path() / "foo") == 1);
    ec = std::error_code(42, std::system_category());
    EXPECT_TRUE(fs::hard_link_count(t.path() / "foo", ec) == 1);
    EXPECT_TRUE(!ec);
    EXPECT_THROW(fs::hard_link_count(t.path() / "bar"), fs::filesystem_error);
    EXPECT_NO_THROW(fs::hard_link_count(t.path() / "bar", ec));
    EXPECT_TRUE(ec);
    ec.clear();
}

static fs::file_time_type timeFromString(const std::string &str) {
    abel::abel_time at;
    std::string err;
    abel::parse_time("%Y-%m-%dT%H:%M:%S", str, &at, &err);
    struct ::tm tm = abel::utc_tm(at);
    /*
    ::memset(&tm, 0, sizeof(::tm));
    std::istringstream is(str);
    is >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    if (is.fail()) {
        throw std::exception();
    }
     */
    return from_time_t<fs::file_time_type>(std::mktime(&tm));
}

TEST(fs, last_write_time) {
    TemporaryDirectory t(TempOpt::change_path);
    std::error_code ec;
    fs::file_time_type ft;
    generateFile("foo");
    auto now = fs::file_time_type::clock::now();
    EXPECT_TRUE(
            std::abs(std::chrono::duration_cast<std::chrono::seconds>(fs::last_write_time(t.path()) - now).count()) <
            3);
    EXPECT_TRUE(
            std::abs(std::chrono::duration_cast<std::chrono::seconds>(fs::last_write_time("foo") - now).count()) < 3);
    EXPECT_THROW(fs::last_write_time("bar"), fs::filesystem_error);
    EXPECT_NO_THROW(ft = fs::last_write_time("bar", ec));
    EXPECT_TRUE(ft == fs::file_time_type::min());
    EXPECT_TRUE(ec);
    ec.clear();
    if (is_symlink_creation_supported()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        fs::create_symlink("foo", "foo2");
        ft = fs::last_write_time("foo");
// checks that the time of the symlink is fetched
        EXPECT_TRUE(ft == fs::last_write_time("foo2"));
    }
    auto nt = timeFromString("2015-10-21T04:30:00");
    EXPECT_NO_THROW(fs::last_write_time(t.path() / "foo", nt));
    EXPECT_TRUE(
            std::abs(std::chrono::duration_cast<std::chrono::seconds>(fs::last_write_time("foo") - nt).count()) < 1);
    nt = timeFromString("2015-10-21T04:29:00");
    EXPECT_NO_THROW(fs::last_write_time("foo", nt, ec));
    EXPECT_TRUE(
            std::abs(std::chrono::duration_cast<std::chrono::seconds>(fs::last_write_time("foo") - nt).count()) < 1);
    EXPECT_TRUE(!ec);
    EXPECT_THROW(fs::last_write_time("bar", nt), fs::filesystem_error);
    EXPECT_NO_THROW(fs::last_write_time("bar", nt, ec));
    EXPECT_TRUE(ec);
}

TEST(fs, permissions) {
    TemporaryDirectory t(TempOpt::change_path);
    std::error_code ec;
    generateFile("foo", 512);
    auto allWrite = fs::perms::owner_write | fs::perms::group_write | fs::perms::others_write;
    EXPECT_NO_THROW(fs::permissions("foo", allWrite, fs::perm_options::remove));
    EXPECT_TRUE((fs::status("foo").permissions() & fs::perms::owner_write) != fs::perms::owner_write);
    EXPECT_THROW(fs::resize_file("foo", 1024), fs::filesystem_error);
    EXPECT_EQ(fs::file_size("foo"), 512);
    EXPECT_NO_THROW(fs::permissions("foo", fs::perms::owner_write, fs::perm_options::add));
    EXPECT_TRUE((fs::status("foo").permissions() & fs::perms::owner_write) == fs::perms::owner_write);
    EXPECT_NO_THROW(fs::resize_file("foo", 2048));
    EXPECT_TRUE(fs::file_size("foo") == 2048);
    EXPECT_THROW(fs::permissions("bar", fs::perms::owner_write, fs::perm_options::add), fs::filesystem_error);
    EXPECT_NO_THROW(fs::permissions("bar", fs::perms::owner_write, fs::perm_options::add, ec));
    EXPECT_TRUE(ec);
    EXPECT_THROW(fs::permissions("bar", fs::perms::owner_write, static_cast<fs::perm_options>(0)),
                 fs::filesystem_error);
}

TEST(fs, proximate) {
    std::error_code ec;
    EXPECT_TRUE(fs::proximate("/a/d", "/a/b/c") == "../../d");
    EXPECT_TRUE(fs::proximate("/a/d", "/a/b/c", ec) == "../../d");
    EXPECT_TRUE(!ec);
    EXPECT_TRUE(fs::proximate("/a/b/c", "/a/d") == "../b/c");
    EXPECT_TRUE(fs::proximate("/a/b/c", "/a/d", ec) == "../b/c");
    EXPECT_TRUE(!ec);
    EXPECT_TRUE(fs::proximate("a/b/c", "a") == "b/c");
    EXPECT_TRUE(fs::proximate("a/b/c", "a", ec) == "b/c");
    EXPECT_TRUE(!ec);
    EXPECT_TRUE(fs::proximate("a/b/c", "a/b/c/x/y") == "../..");
    EXPECT_TRUE(fs::proximate("a/b/c", "a/b/c/x/y", ec) == "../..");
    EXPECT_TRUE(!ec);
    EXPECT_TRUE(fs::proximate("a/b/c", "a/b/c") == ".");
    EXPECT_TRUE(fs::proximate("a/b/c", "a/b/c", ec) == ".");
    EXPECT_TRUE(!ec);
    EXPECT_TRUE(fs::proximate("a/b", "c/d") == "../../a/b");
    EXPECT_TRUE(fs::proximate("a/b", "c/d", ec) == "../../a/b");
    EXPECT_TRUE(!ec);
#ifndef ABEL_PLATFORM_WINDOWS
    if (has_host_root_name_support()) {
        EXPECT_TRUE(fs::proximate("//host1/a/d", "//host2/a/b/c") == "//host1/a/d");
        EXPECT_TRUE(fs::proximate("//host1/a/d", "//host2/a/b/c", ec) == "//host1/a/d");
        EXPECT_TRUE(!ec);
    }
#endif
}

TEST(fs, read_symlink) {
    if (is_symlink_creation_supported()) {
        TemporaryDirectory t(TempOpt::change_path);
        std::error_code ec;
        generateFile("foo");
        fs::create_symlink(t.path() / "foo", "bar");
        EXPECT_TRUE(fs::read_symlink("bar") == t.path() / "foo");
        EXPECT_TRUE(fs::read_symlink("bar", ec) == t.path() / "foo");
        EXPECT_TRUE(!ec);
        EXPECT_THROW(fs::read_symlink("foobar"), fs::filesystem_error);
        EXPECT_TRUE(fs::read_symlink("foobar", ec) == fs::path());
        EXPECT_TRUE(ec);
    }
}

TEST(fs, relative) {
    EXPECT_TRUE(fs::relative("/a/d", "/a/b/c") == "../../d");
    EXPECT_TRUE(fs::relative("/a/b/c", "/a/d") == "../b/c");
    EXPECT_TRUE(fs::relative("a/b/c", "a") == "b/c");
    EXPECT_TRUE(fs::relative("a/b/c", "a/b/c/x/y") == "../..");
    EXPECT_TRUE(fs::relative("a/b/c", "a/b/c") == ".");
    EXPECT_TRUE(fs::relative("a/b", "c/d") == "../../a/b");
    std::error_code ec;
    EXPECT_TRUE(fs::relative(fs::current_path() / "foo", ec) == "foo");
    EXPECT_TRUE(!ec);
}

TEST(fs, remove) {
    TemporaryDirectory t(TempOpt::change_path);
    std::error_code ec;
    generateFile("foo");
    EXPECT_TRUE(fs::remove("foo"));
    EXPECT_TRUE(!fs::exists("foo"));
    EXPECT_TRUE(!fs::remove("foo"));
    generateFile("foo");
    EXPECT_TRUE(fs::remove("foo", ec));
    EXPECT_TRUE(!fs::exists("foo"));
    if (is_symlink_creation_supported()) {
        generateFile("foo");
        fs::create_symlink("foo", "bar");
        EXPECT_TRUE(fs::exists(fs::symlink_status("bar")));
        EXPECT_TRUE(fs::remove("bar", ec));
        EXPECT_TRUE(fs::exists("foo"));
        EXPECT_TRUE(!fs::exists(fs::symlink_status("bar")));
    }
    EXPECT_TRUE(!fs::remove("bar"));
    EXPECT_TRUE(!fs::remove("bar", ec));
    EXPECT_TRUE(!ec);
}

TEST(fs, remove_all) {
    TemporaryDirectory t(TempOpt::change_path);
    std::error_code ec;
    generateFile("foo");
    EXPECT_TRUE(fs::remove_all("foo", ec) == 1);
    EXPECT_TRUE(!ec);
    ec.clear();
    EXPECT_TRUE(fs::directory_iterator(t.path()) == fs::directory_iterator());
    fs::create_directories("dir1/dir1a");
    fs::create_directories("dir1/dir1b");
    generateFile("dir1/dir1a/f1");
    generateFile("dir1/dir1b/f2");
    EXPECT_NO_THROW(fs::remove_all("dir1/non-existing", ec));
    EXPECT_TRUE(!ec);
    EXPECT_TRUE(fs::remove_all("dir1/non-existing", ec) == 0);
    EXPECT_TRUE(fs::remove_all("dir1") == 5);
    EXPECT_TRUE(fs::directory_iterator(t.path()) == fs::directory_iterator());
}

TEST(fs, rename) {
    TemporaryDirectory t(TempOpt::change_path);
    std::error_code ec;
    generateFile("foo", 123);
    fs::create_directory("dir1");
    EXPECT_NO_THROW(fs::rename("foo", "bar"));
    EXPECT_TRUE(!fs::exists("foo"));
    EXPECT_TRUE(fs::exists("bar"));
    EXPECT_NO_THROW(fs::rename("dir1", "dir2"));
    EXPECT_TRUE(fs::exists("dir2"));
    generateFile("foo2", 42);
    EXPECT_NO_THROW(fs::rename("bar", "foo2"));
    EXPECT_TRUE(fs::exists("foo2"));
    EXPECT_TRUE(fs::file_size("foo2") == 123u);
    EXPECT_TRUE(!fs::exists("bar"));
    EXPECT_NO_THROW(fs::rename("foo2", "foo", ec));
    EXPECT_TRUE(!ec);
    EXPECT_THROW(fs::rename("foobar", "barfoo"), fs::filesystem_error);
    EXPECT_NO_THROW(fs::rename("foobar", "barfoo", ec));
    EXPECT_TRUE(ec);
    EXPECT_TRUE(!fs::exists("barfoo"));
}

TEST(fs, resize_file) {
    TemporaryDirectory t(TempOpt::change_path);
    std::error_code ec;
    generateFile("foo", 1024);
    EXPECT_TRUE(fs::file_size("foo") == 1024);
    EXPECT_NO_THROW(fs::resize_file("foo", 2048));
    EXPECT_TRUE(fs::file_size("foo") == 2048);
    EXPECT_NO_THROW(fs::resize_file("foo", 1000, ec));
    EXPECT_TRUE(!ec);
    EXPECT_TRUE(fs::file_size("foo") == 1000);
    EXPECT_THROW(fs::resize_file("bar", 2048), fs::filesystem_error);
    EXPECT_TRUE(!fs::exists("bar"));
    EXPECT_NO_THROW(fs::resize_file("bar", 4096, ec));
    EXPECT_TRUE(ec);
    EXPECT_TRUE(!fs::exists("bar"));
}

TEST(fs, space) {
    {
        fs::space_info si;
        EXPECT_NO_THROW(si = fs::space(fs::current_path()));
        EXPECT_TRUE(si.capacity > 1024 * 1024);
        EXPECT_TRUE(si.capacity > si.free);
        EXPECT_TRUE(si.free >= si.available);
    }
    {
        std::error_code ec;
        fs::space_info si;
        EXPECT_NO_THROW(si = fs::space(fs::current_path(), ec));
        EXPECT_TRUE(si.capacity > 1024 * 1024);
        EXPECT_TRUE(si.capacity > si.free);
        EXPECT_TRUE(si.free >= si.available);
        EXPECT_TRUE(!ec);
    }
    {
        std::error_code ec;
        fs::space_info si;
        EXPECT_NO_THROW(si = fs::space("foobar42", ec));
        EXPECT_TRUE(si.capacity == static_cast<uintmax_t>(-1));
        EXPECT_TRUE(si.free == static_cast<uintmax_t>(-1));
        EXPECT_TRUE(si.available == static_cast<uintmax_t>(-1));
        EXPECT_TRUE(ec);
    }
    EXPECT_THROW(fs::space("foobar42"), fs::filesystem_error);
}

TEST(fs, status) {
    TemporaryDirectory t(TempOpt::change_path);
    std::error_code ec;
    fs::file_status fs;
    EXPECT_NO_THROW(fs = fs::status("foo"));
    EXPECT_TRUE(fs.type() == fs::file_type::not_found);
    EXPECT_TRUE(fs.permissions() == fs::perms::unknown);
    EXPECT_NO_THROW(fs = fs::status("bar", ec));
    EXPECT_TRUE(fs.type() == fs::file_type::not_found);
    EXPECT_TRUE(fs.permissions() == fs::perms::unknown);
    EXPECT_TRUE(ec);
    ec.clear();
    fs = fs::status(t.path());
    EXPECT_TRUE(fs.type() == fs::file_type::directory);
    EXPECT_TRUE((fs.permissions() & (fs::perms::owner_read | fs::perms::owner_write))
                == (fs::perms::owner_read | fs::perms::owner_write));
    generateFile("foobar");
    fs = fs::status(t.path() / "foobar");
    EXPECT_TRUE(fs.type() == fs::file_type::regular);
    EXPECT_TRUE((fs.permissions() & (fs::perms::owner_read | fs::perms::owner_write))
                == (fs::perms::owner_read | fs::perms::owner_write));
    if (is_symlink_creation_supported()) {
        fs::create_symlink(t.path() / "foobar", t.path() / "barfoo");
        fs = fs::status(t.path() / "barfoo");
        EXPECT_TRUE(fs.type() == fs::file_type::regular);
        EXPECT_TRUE((fs.permissions() & (fs::perms::owner_read | fs::perms::owner_write))
                    == (fs::perms::owner_read | fs::perms::owner_write));
    }
}

TEST(fs, status_known) {
    EXPECT_TRUE(!fs::status_known(fs::file_status()));
    EXPECT_TRUE(fs::status_known(fs::file_status(fs::file_type::not_found)));
    EXPECT_TRUE(fs::status_known(fs::file_status(fs::file_type::regular)));
    EXPECT_TRUE(fs::status_known(fs::file_status(fs::file_type::directory)));
    EXPECT_TRUE(fs::status_known(fs::file_status(fs::file_type::symlink)));
    EXPECT_TRUE(fs::status_known(fs::file_status(fs::file_type::character)));
    EXPECT_TRUE(fs::status_known(fs::file_status(fs::file_type::fifo)));
    EXPECT_TRUE(fs::status_known(fs::file_status(fs::file_type::socket)));
    EXPECT_TRUE(fs::status_known(fs::file_status(fs::file_type::unknown)));
}

TEST(fs, symlink_status) {
    TemporaryDirectory t(TempOpt::change_path);
    std::error_code ec;
    fs::file_status fs;
    EXPECT_NO_THROW(fs = fs::symlink_status("foo"));
    EXPECT_TRUE(fs.type() == fs::file_type::not_found);
    EXPECT_TRUE(fs.permissions() == fs::perms::unknown);
    EXPECT_NO_THROW(fs = fs::symlink_status("bar", ec));
    EXPECT_TRUE(fs.type() == fs::file_type::not_found);
    EXPECT_TRUE(fs.permissions() == fs::perms::unknown);
    EXPECT_TRUE(ec);
    ec.clear();
    fs = fs::symlink_status(t.path());
    EXPECT_TRUE(fs.type() == fs::file_type::directory);
    EXPECT_TRUE((fs.permissions() & (fs::perms::owner_read | fs::perms::owner_write))
                == (fs::perms::owner_read | fs::perms::owner_write));
    generateFile("foobar");
    fs = fs::symlink_status(t.path() / "foobar");
    EXPECT_TRUE(fs.type() == fs::file_type::regular);
    EXPECT_TRUE((fs.permissions() & (fs::perms::owner_read | fs::perms::owner_write))
                == (fs::perms::owner_read | fs::perms::owner_write));
    if (is_symlink_creation_supported()) {
        fs::create_symlink(t.path() / "foobar", t.path() / "barfoo");
        fs = fs::symlink_status(t.path() / "barfoo");
        EXPECT_TRUE(fs.type() == fs::file_type::symlink);
    }
}

TEST(fs, temporary_directory_path) {
    std::error_code ec;
    EXPECT_NO_THROW(fs::exists(fs::temp_directory_path()));
    EXPECT_NO_THROW(fs::exists(fs::temp_directory_path(ec)));
    EXPECT_TRUE(!fs::temp_directory_path().empty());
    EXPECT_TRUE(!ec);
}

TEST(fs, weakly_canonical) {
    EXPECT_TRUE("This might fail on std::implementations that return fs::current_path() for fs::canonical(\"\")");
    EXPECT_TRUE(fs::weakly_canonical("") == ".");
    if (fs::weakly_canonical("") == ".") {
        EXPECT_TRUE(fs::weakly_canonical("foo/bar") == "foo/bar");
        EXPECT_TRUE(fs::weakly_canonical("foo/./bar") == "foo/bar");
        EXPECT_TRUE(fs::weakly_canonical("foo/../bar") == "bar");
    } else {
        EXPECT_TRUE(fs::weakly_canonical("foo/bar") == fs::current_path() / "foo/bar");
        EXPECT_TRUE(fs::weakly_canonical("foo/./bar") == fs::current_path() / "foo/bar");
        EXPECT_TRUE(fs::weakly_canonical("foo/../bar") == fs::current_path() / "bar");
    }

    {
        TemporaryDirectory t(TempOpt::change_path);
        auto dir = t.path() / "d0";
        fs::create_directories(dir / "d1");
        generateFile(dir / "f0");
        fs::path rel(dir.filename());
        EXPECT_TRUE(fs::weakly_canonical(dir) == dir);
        EXPECT_TRUE(fs::weakly_canonical(rel) == dir);
        EXPECT_TRUE(fs::weakly_canonical(dir / "f0") == dir / "f0");
        EXPECT_TRUE(fs::weakly_canonical(dir / "f0/") == dir / "f0/");
        EXPECT_TRUE(fs::weakly_canonical(dir / "f1") == dir / "f1");
        EXPECT_TRUE(fs::weakly_canonical(rel / "f0") == dir / "f0");
        EXPECT_TRUE(fs::weakly_canonical(rel / "f0/") == dir / "f0/");
        EXPECT_TRUE(fs::weakly_canonical(rel / "f1") == dir / "f1");
        EXPECT_TRUE(fs::weakly_canonical(rel / "./f0") == dir / "f0");
        EXPECT_TRUE(fs::weakly_canonical(rel / "./f1") == dir / "f1");
        EXPECT_TRUE(fs::weakly_canonical(rel / "d1/../f0") == dir / "f0");
        EXPECT_TRUE(fs::weakly_canonical(rel / "d1/../f1") == dir / "f1");
        EXPECT_TRUE(fs::weakly_canonical(rel / "d1/../f1/../f2") == dir / "f2");
    }
}

TEST(fs, support_string_view) {
//#if __cpp_lib_string_view
    std::string p("foo/bar");
    abel::string_view sv(p);
    EXPECT_TRUE(fs::path(sv, fs::path::format::generic_format).generic_string() == "foo/bar");
    fs::path p2("fo");
    p2 += abel::string_view("o");
    EXPECT_TRUE(p2 == "foo");
    EXPECT_TRUE(p2.compare(abel::string_view("foo")) == 0);
//#else
    //   EXPECT_TRUE("std::string_view specific tests are empty without std::string_view.");
//#endif
}

TEST(fs, filename_support) {
#ifdef ABEL_PLATFORM_WINDOWS
    TemporaryDirectory t(TempOpt::change_path);
        char c = 'A';
        fs::path dir = "\\\\?\\" + fs::current_path().u8string();
        for (; c <= 'Z'; ++c) {
            std::string part = std::string(16, c);
            dir /= part;
            EXPECT_NO_THROW(fs::create_directory(dir));
            EXPECT_TRUE(fs::exists(dir));
            generateFile(dir / "f0");
            EXPECT_TRUE(fs::exists(dir / "f0"));
            std::string native = dir.u8string();
            if (native.substr(0, 4) == "\\\\?\\") {
                break;
            }
        }
        EXPECT_TRUE(c <= 'Z');
#else
    EXPECT_TRUE("Windows specific tests are empty on non-Windows systems.");
#endif
}

TEST(fs, unc) {
#ifdef ABEL_PLATFORM_WINDOWS
    std::error_code ec;
        fs::path p(R"(\\localhost\c$\Windows)");
        auto symstat = fs::symlink_status(p, ec);
        EXPECT_TRUE(!ec);
        auto p2 = fs::canonical(p, ec);
        EXPECT_TRUE(!ec);
        EXPECT_TRUE(p2 == p);

        std::vector<fs::path> variants = {
            R"(C:\Windows\notepad.exe)",
            R"(\\.\C:\Windows\notepad.exe)",
            R"(\\?\C:\Windows\notepad.exe)",
            R"(\??\C:\Windows\notepad.exe)",
            R"(\\?\HarddiskVolume1\Windows\notepad.exe)",
            R"(\\?\Harddisk0Partition1\Windows\notepad.exe)",
            R"(\\.\GLOBALROOT\Device\HarddiskVolume1\Windows\notepad.exe)",
            R"(\\?\GLOBALROOT\Device\Harddisk0\Partition1\Windows\notepad.exe)",
            R"(\\?\Volume{e8a4a89d-0000-0000-0000-100000000000}\Windows\notepad.exe)",
            R"(\\LOCALHOST\C$\Windows\notepad.exe)",
            R"(\\?\UNC\C$\Windows\notepad.exe)",
            R"(\\?\GLOBALROOT\Device\Mup\C$\Windows\notepad.exe)",
        };
        for (auto pt : variants) {
            std::cerr << pt.string() << " - " << pt.root_name() << ", " << pt.root_path() << ": " << iterateResult(pt) << std::endl;
        }
#else
    EXPECT_TRUE("Windows specific tests are empty on non-Windows systems.");
#endif
}
