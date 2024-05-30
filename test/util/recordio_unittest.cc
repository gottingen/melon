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


#include <map>
#include <gtest/gtest.h>
#include "melon/utility/recordio.h"
#include <melon/utility/fast_rand.h>
#include <melon/utility/string_printf.h>
#include <melon/utility/file_util.h>

namespace {

class StringReader : public mutil::IReader {
public:
    StringReader(const std::string& str,
                 bool report_eagain_on_end = false)
        : _str(str)
        , _offset(0)
        , _report_eagain_on_end(report_eagain_on_end) {}

    ssize_t ReadV(const iovec* iov, int iovcnt) override {
        size_t total_nc = 0;
        for (int i = 0; i < iovcnt; ++i) {
            void* dst = iov[i].iov_base;
            size_t len = iov[i].iov_len;
            size_t remain = _str.size() - _offset;
            size_t nc = std::min(len, remain);
            memcpy(dst, _str.data() + _offset, nc);
            _offset += nc;
            total_nc += nc;
            if (_offset == _str.size()) {
                break;
            }
        }
        if (_report_eagain_on_end && total_nc == 0) {
            errno = EAGAIN;
            return -1;
        }
        return total_nc;
    }
private:
    std::string _str;
    size_t _offset;
    bool _report_eagain_on_end;
};

class StringWriter : public mutil::IWriter {
public:
    ssize_t WriteV(const iovec* iov, int iovcnt) override {
        const size_t old_size = _str.size();
        for (int i = 0; i < iovcnt; ++i) {
            _str.append((char*)iov[i].iov_base, iov[i].iov_len);
        }
        return _str.size() - old_size;
    }
    const std::string& str() const { return _str; }
private:
    std::string _str;
};

TEST(RecordIOTest, empty_record) {
    mutil::Record r;
    ASSERT_EQ((size_t)0, r.MetaCount());
    ASSERT_TRUE(r.Meta("foo") == NULL);
    ASSERT_FALSE(r.RemoveMeta("foo"));
    ASSERT_TRUE(r.Payload().empty());
    ASSERT_TRUE(r.MutablePayload()->empty());
}

TEST(RecordIOTest, manipulate_record) {
    mutil::Record r1;
    ASSERT_EQ((size_t)0, r1.MetaCount());
    mutil::IOBuf* foo_val = r1.MutableMeta("foo");
    ASSERT_EQ((size_t)1, r1.MetaCount());
    ASSERT_TRUE(foo_val->empty());
    foo_val->append("foo_data");
    ASSERT_EQ(foo_val, r1.MutableMeta("foo"));
    ASSERT_EQ((size_t)1, r1.MetaCount());
    ASSERT_EQ("foo_data", *foo_val);
    ASSERT_EQ(foo_val, r1.Meta("foo"));

    mutil::IOBuf* bar_val = r1.MutableMeta("bar");
    ASSERT_EQ((size_t)2, r1.MetaCount());
    ASSERT_TRUE(bar_val->empty());
    bar_val->append("bar_data");
    ASSERT_EQ(bar_val, r1.MutableMeta("bar"));
    ASSERT_EQ((size_t)2, r1.MetaCount());
    ASSERT_EQ("bar_data", *bar_val);
    ASSERT_EQ(bar_val, r1.Meta("bar"));

    mutil::Record r2 = r1;

    ASSERT_TRUE(r1.RemoveMeta("foo"));
    ASSERT_EQ((size_t)1, r1.MetaCount());
    ASSERT_TRUE(r1.Meta("foo") == NULL);

    ASSERT_EQ(foo_val, r2.Meta("foo"));
    ASSERT_EQ("foo_data", *foo_val);
}

TEST(RecordIOTest, invalid_name) {
    char name[258];
    for (size_t i = 0; i < sizeof(name); ++i) {
        name[i] = 'a';
    }
    name[sizeof(name) - 1] = 0;
    mutil::Record r;
    ASSERT_EQ(NULL, r.MutableMeta(name));
}

TEST(RecordIOTest, write_read_basic) {
    StringWriter sw;
    mutil::RecordWriter rw(&sw);

    mutil::Record src;
    ASSERT_EQ(0, rw.Write(src));

    mutil::IOBuf* foo_val = src.MutableMeta("foo");
    foo_val->append("foo_data");
    ASSERT_EQ(0, rw.Write(src));

    mutil::IOBuf* bar_val = src.MutableMeta("bar");
    bar_val->append("bar_data");
    ASSERT_EQ(0, rw.Write(src));

    src.MutablePayload()->append("payload_data");
    ASSERT_EQ(0, rw.Write(src));

    ASSERT_EQ(0, rw.Flush());
    std::cout << "len=" << sw.str().size()
              << " content=" << mutil::PrintedAsBinary(sw.str(), 256) << std::endl;

    StringReader sr(sw.str());
    mutil::RecordReader rr(&sr);
    mutil::Record r1;
    ASSERT_TRUE(rr.ReadNext(&r1));
    ASSERT_EQ(0, rr.last_error());
    ASSERT_EQ((size_t)0, r1.MetaCount());
    ASSERT_TRUE(r1.Payload().empty());

    mutil::Record r2;
    ASSERT_TRUE(rr.ReadNext(&r2));
    ASSERT_EQ(0, rr.last_error());
    ASSERT_EQ((size_t)1, r2.MetaCount());
    ASSERT_EQ("foo", r2.MetaAt(0).name);
    ASSERT_EQ("foo_data", *r2.MetaAt(0).data);
    ASSERT_TRUE(r2.Payload().empty());

    mutil::Record r3;
    ASSERT_TRUE(rr.ReadNext(&r3));
    ASSERT_EQ(0, rr.last_error());
    ASSERT_EQ((size_t)2, r3.MetaCount());
    ASSERT_EQ("foo", r3.MetaAt(0).name);
    ASSERT_EQ("foo_data", *r3.MetaAt(0).data);
    ASSERT_EQ("bar", r3.MetaAt(1).name);
    ASSERT_EQ("bar_data", *r3.MetaAt(1).data);
    ASSERT_TRUE(r3.Payload().empty());

    mutil::Record r4;
    ASSERT_TRUE(rr.ReadNext(&r4));
    ASSERT_EQ(0, rr.last_error());
    ASSERT_EQ((size_t)2, r4.MetaCount());
    ASSERT_EQ("foo", r4.MetaAt(0).name);
    ASSERT_EQ("foo_data", *r4.MetaAt(0).data);
    ASSERT_EQ("bar", r4.MetaAt(1).name);
    ASSERT_EQ("bar_data", *r4.MetaAt(1).data);
    ASSERT_EQ("payload_data", r4.Payload());

    ASSERT_FALSE(rr.ReadNext(NULL));
    ASSERT_EQ((int)mutil::RecordReader::END_OF_READER, rr.last_error());
    ASSERT_EQ(sw.str().size(), rr.offset());
}

TEST(RecordIOTest, incomplete_reader) {
    StringWriter sw;
    mutil::RecordWriter rw(&sw);

    mutil::Record src;
    mutil::IOBuf* foo_val = src.MutableMeta("foo");
    foo_val->append("foo_data");
    ASSERT_EQ(0, rw.Write(src));

    mutil::IOBuf* bar_val = src.MutableMeta("bar");
    bar_val->append("bar_data");
    ASSERT_EQ(0, rw.Write(src));

    ASSERT_EQ(0, rw.Flush());
    std::string data = sw.str();
    std::cout << "len=" << data.size()
              << " content=" << mutil::PrintedAsBinary(data, 256) << std::endl;

    StringReader sr(data, true);
    mutil::RecordReader rr(&sr);

    mutil::Record r2;
    ASSERT_TRUE(rr.ReadNext(&r2));
    ASSERT_EQ(0, rr.last_error());
    ASSERT_EQ((size_t)1, r2.MetaCount());
    ASSERT_EQ("foo", r2.MetaAt(0).name);
    ASSERT_EQ("foo_data", *r2.MetaAt(0).data);
    ASSERT_TRUE(r2.Payload().empty());

    mutil::Record r3;
    ASSERT_TRUE(rr.ReadNext(&r3));
    ASSERT_EQ(0, rr.last_error());
    ASSERT_EQ((size_t)2, r3.MetaCount());
    ASSERT_EQ("foo", r3.MetaAt(0).name);
    ASSERT_EQ("foo_data", *r3.MetaAt(0).data);
    ASSERT_EQ("bar", r3.MetaAt(1).name);
    ASSERT_EQ("bar_data", *r3.MetaAt(1).data);
    ASSERT_TRUE(r3.Payload().empty());

    ASSERT_FALSE(rr.ReadNext(NULL));
    ASSERT_EQ(EAGAIN, rr.last_error());
    ASSERT_EQ(sw.str().size(), rr.offset());
}

static std::string rand_string(int min_len, int max_len) {
    const int len = mutil::fast_rand_in(min_len, max_len);
    std::string str;
    str.reserve(len);
    for (int i = 0; i < len; ++i) {
        str.push_back(mutil::fast_rand_in('a', 'Z'));
    }
    return str;
}

TEST(RecordIOTest, write_read_random) {
    StringWriter sw;
    mutil::RecordWriter rw(&sw);

    const int N = 1024;
    std::vector<std::pair<std::string, std::string>> name_value_list;
    size_t nbytes = 0;
    std::map<int, size_t> breaking_offsets;
    for (int i = 0; i < N; ++i) {
        mutil::Record src;
        std::string value = rand_string(10, 20);
        std::string name = mutil::string_printf("name_%d_", i) + value;
        src.MutableMeta(name)->append(value);
        ASSERT_EQ(0, rw.Write(src));
        if (mutil::fast_rand_less_than(70) == 0) {
            breaking_offsets[i] = nbytes;
        } else {
            name_value_list.push_back(std::make_pair(name, value));
        }
        nbytes += src.ByteSize();
    }
    ASSERT_EQ(0, rw.Flush());
    std::string str = sw.str();
    ASSERT_EQ(nbytes, str.size());
    // break some records
    int break_idx = 0;
    for (auto it = breaking_offsets.begin(); it != breaking_offsets.end(); ++it) {
        switch (break_idx++ % 10) {
        case 0:
            str[it->second] = 'r';
            break;
        case 1:
            str[it->second + 1] = 'd';
            break;
        case 2:
            str[it->second + 2] = 'i';
            break;
        case 3:
            str[it->second + 3] = 'o';
            break;
        case 4:
            ++str[it->second + 4];
            break;
        case 5:
            str[it->second + 4] = 8;
            break;
        case 6:
            ++str[it->second + 5];
            break;
        case 7:
            ++str[it->second + 6];
            break;
        case 8:
            ++str[it->second + 7];
            break;
        case 9:
            ++str[it->second + 8];
            break;
        default:
            ASSERT_TRUE(false) << "never";
        }
    }
    ASSERT_EQ((size_t)N - breaking_offsets.size(), name_value_list.size());
    std::cout << "sw.size=" << str.size()
              << " nbreak=" << breaking_offsets.size() << std::endl;

    StringReader sr(str);
    ASSERT_LT(0, mutil::WriteFile(mutil::FilePath("recordio_ref.io"), str.data(), str.size()));
    mutil::RecordReader rr(&sr);
    size_t j = 0;
    mutil::Record r;
    for (; rr.ReadNext(&r); ++j) {
        ASSERT_LT(j, name_value_list.size());
        ASSERT_EQ((size_t)1, r.MetaCount());
        ASSERT_EQ(name_value_list[j].first, r.MetaAt(0).name) << j;
        ASSERT_EQ(name_value_list[j].second, *r.MetaAt(0).data);
    }
    ASSERT_EQ((int)mutil::RecordReader::END_OF_READER, rr.last_error());
    ASSERT_EQ(j, name_value_list.size());
    ASSERT_LE(str.size() - rr.offset(), 3u);
}

} // namespace
