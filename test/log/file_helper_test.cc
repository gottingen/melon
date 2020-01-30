
#include <test/testing/log_includes.h>

using abel::details::file_helper;
using abel::details::log_msg;

static const std::string target_filename = "logs/file_helper_test.txt";

static void write_with_helper (file_helper &helper, size_t howmany) {
    log_msg msg;
    fmt::memory_buffer formatted;
    fmt::format_to(formatted, "{}", std::string(howmany, '1'));
    helper.write(formatted);
    helper.flush();
}

TEST(fileHeler, filehelperfilename) {
    prepare_logdir();

    file_helper helper;
    helper.open(target_filename);
    EXPECT_TRUE(helper.filename() == target_filename);
}

TEST(filehelpersize, filehelpersize) {
    prepare_logdir();
    size_t expected_size = 123;
    {
        file_helper helper;
        helper.open(target_filename);
        write_with_helper(helper, expected_size);
        EXPECT_TRUE(static_cast<size_t>(helper.size()) == expected_size);
    }
    EXPECT_TRUE(get_filesize(target_filename) == expected_size);
}

TEST(filehelper, filehelperexists) {
    prepare_logdir();
    EXPECT_TRUE(!file_helper::file_exists(target_filename));
    file_helper helper;
    helper.open(target_filename);
    EXPECT_TRUE(file_helper::file_exists(target_filename));
}

TEST(filehlper, filehelperreopen) {
    prepare_logdir();
    file_helper helper;
    helper.open(target_filename);
    write_with_helper(helper, 12);
    EXPECT_TRUE(helper.size() == 12);
    helper.reopen(true);
    EXPECT_TRUE(helper.size() == 0);
}

TEST(filehelper, filehelperreopen2) {
    prepare_logdir();
    size_t expected_size = 14;
    file_helper helper;
    helper.open(target_filename);
    write_with_helper(helper, expected_size);
    EXPECT_TRUE(helper.size() == expected_size);
    helper.reopen(false);
    EXPECT_TRUE(helper.size() == expected_size);
}

static void test_split_ext (const char *fname, const char *expect_base, const char *expect_ext) {
    abel::filename_t filename(fname);
    abel::filename_t expected_base(expect_base);
    abel::filename_t expected_ext(expect_ext);

#ifdef _WIN32 // replace folder sep
    std::replace(filename.begin(), filename.end(), '/', '\\');
    std::replace(expected_base.begin(), expected_base.end(), '/', '\\');
#endif
    abel::filename_t basename, ext;
    std::tie(basename, ext) = file_helper::split_by_extenstion(filename);
    EXPECT_TRUE(basename == expected_base);
    EXPECT_TRUE(ext == expected_ext);
}

TEST(filehelpwe, filehelpersplitbyextenstion) {
    test_split_ext("mylog.txt", "mylog", ".txt");
    test_split_ext(".mylog.txt", ".mylog", ".txt");
    test_split_ext(".mylog", ".mylog", "");
    test_split_ext("/aaa/bb.d/mylog", "/aaa/bb.d/mylog", "");
    test_split_ext("/aaa/bb.d/mylog.txt", "/aaa/bb.d/mylog", ".txt");
    test_split_ext("aaa/bbb/ccc/mylog.txt", "aaa/bbb/ccc/mylog", ".txt");
    test_split_ext("aaa/bbb/ccc/mylog.", "aaa/bbb/ccc/mylog.", "");
    test_split_ext("aaa/bbb/ccc/.mylog.txt", "aaa/bbb/ccc/.mylog", ".txt");
    test_split_ext("/aaa/bbb/ccc/mylog.txt", "/aaa/bbb/ccc/mylog", ".txt");
    test_split_ext("/aaa/bbb/ccc/.mylog", "/aaa/bbb/ccc/.mylog", "");
    test_split_ext("../mylog.txt", "../mylog", ".txt");
    test_split_ext(".././mylog.txt", ".././mylog", ".txt");
    test_split_ext(".././mylog.txt/xxx", ".././mylog.txt/xxx", "");
    test_split_ext("/mylog.txt", "/mylog", ".txt");
    test_split_ext("//mylog.txt", "//mylog", ".txt");
    test_split_ext("", "", "");
    test_split_ext(".", ".", "");
    test_split_ext("..txt", ".", ".txt");
}
