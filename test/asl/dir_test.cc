//
// Created by liyinbin on 2020/1/27.
//

#include <testing/filesystem_test_util.h>
#include <gtest/gtest.h>

TEST(directory, entry) {
    TemporaryDirectory t;
    std::error_code ec;
    auto de = fs::directory_entry(t.path());
    EXPECT_TRUE(de.path() == t.path());
    EXPECT_TRUE((fs::path) de == t.path());
    EXPECT_TRUE(de.exists());
    EXPECT_TRUE(!de.is_block_file());
    EXPECT_TRUE(!de.is_character_file());
    EXPECT_TRUE(de.is_directory());
    EXPECT_TRUE(!de.is_fifo());
    EXPECT_TRUE(!de.is_other());
    EXPECT_TRUE(!de.is_regular_file());
    EXPECT_TRUE(!de.is_socket());
    EXPECT_TRUE(!de.is_symlink());
    EXPECT_TRUE(de.status().type() == fs::file_type::directory);
    ec.clear();
    EXPECT_TRUE(de.status(ec).type() == fs::file_type::directory);
    EXPECT_TRUE(!ec);
    EXPECT_NO_THROW(de.refresh());
    fs::directory_entry none;
    EXPECT_THROW(none.refresh(), fs::filesystem_error);
    ec.clear();
    EXPECT_NO_THROW(none.refresh(ec));
    EXPECT_TRUE(ec);
    EXPECT_THROW(de.assign(""), fs::filesystem_error);
    ec.clear();
    EXPECT_NO_THROW(de.assign("", ec));
    EXPECT_TRUE(ec);
    generateFile(t.path() / "foo", 1234);
    auto now = fs::file_time_type::clock::now();
    EXPECT_NO_THROW(de.assign(t.path() / "foo"));
    EXPECT_NO_THROW(de.assign(t.path() / "foo", ec));
    EXPECT_TRUE(!ec);
    de = fs::directory_entry(t.path() / "foo");
    EXPECT_TRUE(de.path() == t.path() / "foo");
    EXPECT_TRUE(de.exists());
    EXPECT_TRUE(de.exists(ec));
    EXPECT_TRUE(!ec);
    EXPECT_TRUE(!de.is_block_file());
    EXPECT_TRUE(!de.is_block_file(ec));
    EXPECT_TRUE(!ec);
    EXPECT_TRUE(!de.is_character_file());
    EXPECT_TRUE(!de.is_character_file(ec));
    EXPECT_TRUE(!ec);
    EXPECT_TRUE(!de.is_directory());
    EXPECT_TRUE(!de.is_directory(ec));
    EXPECT_TRUE(!ec);
    EXPECT_TRUE(!de.is_fifo());
    EXPECT_TRUE(!de.is_fifo(ec));
    EXPECT_TRUE(!ec);
    EXPECT_TRUE(!de.is_other());
    EXPECT_TRUE(!de.is_other(ec));
    EXPECT_TRUE(!ec);
    EXPECT_TRUE(de.is_regular_file());
    EXPECT_TRUE(de.is_regular_file(ec));
    EXPECT_TRUE(!ec);
    EXPECT_TRUE(!de.is_socket());
    EXPECT_TRUE(!de.is_socket(ec));
    EXPECT_TRUE(!ec);
    EXPECT_TRUE(!de.is_symlink());
    EXPECT_TRUE(!de.is_symlink(ec));
    EXPECT_TRUE(!ec);
    EXPECT_TRUE(de.file_size() == 1234);
    EXPECT_TRUE(de.file_size(ec) == 1234);
    EXPECT_TRUE(std::abs(std::chrono::duration_cast<std::chrono::seconds>(de.last_write_time() - now).count()) < 3);
    ec.clear();
    EXPECT_TRUE(std::abs(std::chrono::duration_cast<std::chrono::seconds>(de.last_write_time(ec) - now).count()) < 3);
    EXPECT_TRUE(!ec);
    EXPECT_TRUE(de.hard_link_count() == 1);
    EXPECT_TRUE(de.hard_link_count(ec) == 1);
    EXPECT_TRUE(!ec);
    EXPECT_THROW(de.replace_filename("bar"), fs::filesystem_error);
    EXPECT_NO_THROW(de.replace_filename("foo"));
    ec.clear();
    EXPECT_NO_THROW(de.replace_filename("bar", ec));
    EXPECT_TRUE(ec);
    auto de2none = fs::directory_entry();
    ec.clear();
    EXPECT_TRUE(de2none.hard_link_count(ec) == static_cast<uintmax_t>(-1));
    EXPECT_THROW(de2none.hard_link_count(), fs::filesystem_error);
    EXPECT_TRUE(ec);
    ec.clear();
    EXPECT_NO_THROW(de2none.last_write_time(ec));
    EXPECT_THROW(de2none.last_write_time(), fs::filesystem_error);
    EXPECT_TRUE(ec);
    ec.clear();
    EXPECT_THROW(de2none.file_size(), fs::filesystem_error);
    EXPECT_TRUE(de2none.file_size(ec) == static_cast<uintmax_t>(-1));
    EXPECT_TRUE(ec);
    ec.clear();
    EXPECT_TRUE(de2none.status().type() == fs::file_type::not_found);
    EXPECT_TRUE(de2none.status(ec).type() == fs::file_type::not_found);
    EXPECT_TRUE(ec);
    generateFile(t.path() / "a");
    generateFile(t.path() / "b");
    auto d1 = fs::directory_entry(t.path() / "a");
    auto d2 = fs::directory_entry(t.path() / "b");
    EXPECT_TRUE(d1 < d2);
    EXPECT_TRUE(!(d2 < d1));
    EXPECT_TRUE(d1 <= d2);
    EXPECT_TRUE(!(d2 <= d1));
    EXPECT_TRUE(d2 > d1);
    EXPECT_TRUE(!(d1 > d2));
    EXPECT_TRUE(d2 >= d1);
    EXPECT_TRUE(!(d1 >= d2));
    EXPECT_TRUE(d1 != d2);
    EXPECT_TRUE(!(d2 != d2));
    EXPECT_TRUE(d1 == d1);
    EXPECT_TRUE(!(d1 == d2));
}

TEST(directory, iterator) {
    {
        TemporaryDirectory t;
        EXPECT_TRUE(fs::directory_iterator(t.path()) == fs::directory_iterator());
        generateFile(t.path() / "test", 1234);
        EXPECT_TRUE(fs::directory_iterator(t.path()) != fs::directory_iterator());
        auto iter = fs::directory_iterator(t.path());
        fs::directory_iterator iter2(iter);
        fs::directory_iterator iter3, iter4;
        iter3 = iter;
        EXPECT_TRUE(iter->path().filename() == "test");
        EXPECT_TRUE(iter2->path().filename() == "test");
        EXPECT_TRUE(iter3->path().filename() == "test");
        iter4 = std::move(iter3);
        EXPECT_TRUE(iter4->path().filename() == "test");
        EXPECT_TRUE(iter->path() == t.path() / "test");
        EXPECT_TRUE(!iter->is_symlink());
        EXPECT_TRUE(iter->is_regular_file());
        EXPECT_TRUE(!iter->is_directory());
        EXPECT_TRUE(iter->file_size() == 1234);
        EXPECT_TRUE(++iter == fs::directory_iterator());
        EXPECT_THROW(fs::directory_iterator(t.path() / "non-existing"), fs::filesystem_error);
        int cnt = 0;
        for (auto de : fs::directory_iterator(t.path())) {
            ++cnt;
        }
        EXPECT_TRUE(cnt == 1);
    }
    if (is_symlink_creation_supported()) {
        TemporaryDirectory t;
        fs::path td = t.path() / "testdir";
        EXPECT_TRUE(fs::directory_iterator(t.path()) == fs::directory_iterator());
        generateFile(t.path() / "test", 1234);
        fs::create_directory(td);
        EXPECT_NO_THROW(fs::create_symlink(t.path() / "test", td / "testlink"));
        std::error_code ec;
        EXPECT_TRUE(fs::directory_iterator(td) != fs::directory_iterator());
        auto iter = fs::directory_iterator(td);
        EXPECT_TRUE(iter->path().filename() == "testlink");
        EXPECT_TRUE(iter->path() == td / "testlink");
        EXPECT_TRUE(iter->is_symlink());
        EXPECT_TRUE(iter->is_regular_file());
        EXPECT_TRUE(!iter->is_directory());
        EXPECT_TRUE(iter->file_size() == 1234);
        EXPECT_TRUE(++iter == fs::directory_iterator());
    }
    {
// Issue #8: check if resources are freed when iterator reaches end()
        TemporaryDirectory t(TempOpt::change_path);
        auto p = fs::path("test/");
        fs::create_directory(p);
        auto iter = fs::directory_iterator(p);
        while (iter != fs::directory_iterator()) {
            ++iter;
        }
        EXPECT_TRUE(fs::remove_all(p) == 1);
        EXPECT_NO_THROW(fs::create_directory(p));
    }
}

TEST(directory, riterator) {
    {
        auto iter = fs::recursive_directory_iterator(".");
        iter.pop();
        EXPECT_TRUE(iter == fs::recursive_directory_iterator());
    }
    {
        TemporaryDirectory t;
        EXPECT_TRUE(fs::recursive_directory_iterator(t.path()) == fs::recursive_directory_iterator());
        generateFile(t.path() / "test", 1234);
        EXPECT_TRUE(fs::recursive_directory_iterator(t.path()) != fs::recursive_directory_iterator());
        auto iter = fs::recursive_directory_iterator(t.path());
        EXPECT_TRUE(iter->path().filename() == "test");
        EXPECT_TRUE(iter->path() == t.path() / "test");
        EXPECT_TRUE(!iter->is_symlink());
        EXPECT_TRUE(iter->is_regular_file());
        EXPECT_TRUE(!iter->is_directory());
        EXPECT_TRUE(iter->file_size() == 1234);
        EXPECT_TRUE(++iter == fs::recursive_directory_iterator());
    }

    {
        TemporaryDirectory t;
        fs::path td = t.path() / "testdir";
        fs::create_directories(td);
        generateFile(td / "test", 1234);
        EXPECT_TRUE(fs::recursive_directory_iterator(t.path()) != fs::recursive_directory_iterator());
        auto iter = fs::recursive_directory_iterator(t.path());

        EXPECT_TRUE(iter->path().filename() == "testdir");
        EXPECT_TRUE(iter->path() == td);
        EXPECT_TRUE(!iter->is_symlink());
        EXPECT_TRUE(!iter->is_regular_file());
        EXPECT_TRUE(iter->is_directory());

        EXPECT_TRUE(++iter != fs::recursive_directory_iterator());

        EXPECT_TRUE(iter->path().filename() == "test");
        EXPECT_TRUE(iter->path() == td / "test");
        EXPECT_TRUE(!iter->is_symlink());
        EXPECT_TRUE(iter->is_regular_file());
        EXPECT_TRUE(!iter->is_directory());
        EXPECT_TRUE(iter->file_size() == 1234);

        EXPECT_TRUE(++iter == fs::recursive_directory_iterator());
    }
    {
        TemporaryDirectory t;
        std::error_code ec;
        EXPECT_TRUE(fs::recursive_directory_iterator(t.path(), fs::directory_options::none)
                    == fs::recursive_directory_iterator());
        EXPECT_TRUE(fs::recursive_directory_iterator(t.path(), fs::directory_options::none, ec)
                    == fs::recursive_directory_iterator());
        EXPECT_TRUE(!ec);
        EXPECT_TRUE(fs::recursive_directory_iterator(t.path(), ec) == fs::recursive_directory_iterator());
        EXPECT_TRUE(!ec);
        generateFile(t.path() / "test");
        fs::recursive_directory_iterator rd1(t.path());
        EXPECT_TRUE(fs::recursive_directory_iterator(rd1) != fs::recursive_directory_iterator());
        fs::recursive_directory_iterator rd2(t.path());
        EXPECT_TRUE(fs::recursive_directory_iterator(std::move(rd2)) != fs::recursive_directory_iterator());
        fs::recursive_directory_iterator rd3(t.path(), fs::directory_options::skip_permission_denied);
        EXPECT_TRUE(rd3.options() == fs::directory_options::skip_permission_denied);
        fs::recursive_directory_iterator rd4;
        rd4 = std::move(rd3);
        EXPECT_TRUE(rd4 != fs::recursive_directory_iterator());
        EXPECT_NO_THROW(++rd4);
        EXPECT_TRUE(rd4 == fs::recursive_directory_iterator());
        fs::recursive_directory_iterator rd5;
        rd5 = rd4;
    }
    {
        TemporaryDirectory t(TempOpt::change_path);
        generateFile("a");
        fs::create_directory("d1");
        fs::create_directory("d1/d2");
        generateFile("d1/b");
        generateFile("d1/c");
        generateFile("d1/d2/d");
        generateFile("e");
        auto iter = fs::recursive_directory_iterator(".");
        std::multimap<std::string, int> result;
        while (iter != fs::recursive_directory_iterator()) {
            result.insert(std::make_pair(iter->path().generic_string(), iter.depth()));
            ++iter;
        }
        std::stringstream os;
        for (auto p : result) {
            os << "[" << p.first << "," << p.second << "],";
        }
        EXPECT_TRUE(os.str() == "[./a,0],[./d1,0],[./d1/b,1],[./d1/c,1],[./d1/d2,1],[./d1/d2/d,2],[./e,0],");
    }
    {
        TemporaryDirectory t(TempOpt::change_path);
        generateFile("a");
        fs::create_directory("d1");
        fs::create_directory("d1/d2");
        generateFile("d1/b");
        generateFile("d1/c");
        generateFile("d1/d2/d");
        generateFile("e");
        std::multiset<std::string> result;
        for (auto de : fs::recursive_directory_iterator(".")) {
            result.insert(de.path().generic_string());
        }
        std::stringstream os;
        for (auto p : result) {
            os << p << ",";
        }
        EXPECT_TRUE(os.str() == "./a,./d1,./d1/b,./d1/c,./d1/d2,./d1/d2/d,./e,");
    }
    {
        TemporaryDirectory t(TempOpt::change_path);
        generateFile("a");
        fs::create_directory("d1");
        fs::create_directory("d1/d2");
        generateFile("d1/d2/b");
        generateFile("e");
        auto iter = fs::recursive_directory_iterator(".");
        std::multimap<std::string, int> result;
        while (iter != fs::recursive_directory_iterator()) {
            result.insert(std::make_pair(iter->path().generic_string(), iter.depth()));
            if (iter->path() == "./d1/d2") {
                iter.disable_recursion_pending();
            }
            ++iter;
        }
        std::stringstream os;
        for (auto p : result) {
            os << "[" << p.first << "," << p.second << "],";
        }
        EXPECT_TRUE(os.str() == "[./a,0],[./d1,0],[./d1/d2,1],[./e,0],");
    }
    {
        TemporaryDirectory t(TempOpt::change_path);
        generateFile("a");
        fs::create_directory("d1");
        fs::create_directory("d1/d2");
        generateFile("d1/d2/b");
        generateFile("e");
        auto iter = fs::recursive_directory_iterator(".");
        std::multimap<std::string, int> result;
        while (iter != fs::recursive_directory_iterator()) {
            result.insert(std::make_pair(iter->path().generic_string(), iter.depth()));
            if (iter->path() == "./d1/d2") {
                iter.pop();
            } else {
                ++iter;
            }
        }
        std::stringstream os;
        for (auto p : result) {
            os << "[" << p.first << "," << p.second << "],";
        }
        EXPECT_TRUE(os.str() == "[./a,0],[./d1,0],[./d1/d2,1],[./e,0],");
    }
}

TEST(directory, absolute) {
    EXPECT_TRUE(fs::absolute("") == fs::current_path() / "");
    EXPECT_TRUE(fs::absolute(fs::current_path()) == fs::current_path());
    EXPECT_TRUE(fs::absolute(".") == fs::current_path() / ".");
    EXPECT_TRUE((fs::absolute("..") == fs::current_path().parent_path()
                 || fs::absolute("..") == fs::current_path() / ".."));
    EXPECT_TRUE(fs::absolute("foo") == fs::current_path() / "foo");
    std::error_code ec;
    EXPECT_TRUE(fs::absolute("", ec) == fs::current_path() / "");
    EXPECT_TRUE(!ec);
    EXPECT_TRUE(fs::absolute("foo", ec) == fs::current_path() / "foo");
    EXPECT_TRUE(!ec);
}
