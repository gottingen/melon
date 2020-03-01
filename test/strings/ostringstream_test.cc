//

#include <abel/strings/internal/ostringstream.h>

#include <memory>
#include <ostream>
#include <string>
#include <type_traits>

#include <gtest/gtest.h>

namespace {

    TEST(OStringStream, IsOStream) {
        static_assert(
                std::is_base_of<std::ostream, abel::strings_internal::OStringStream>(),
                "");
    }

    TEST(OStringStream, ConstructDestroy) {
        {
            abel::strings_internal::OStringStream strm(nullptr);
            EXPECT_EQ(nullptr, strm.str());
        }
        {
            std::string s = "abc";
            {
                abel::strings_internal::OStringStream strm(&s);
                EXPECT_EQ(&s, strm.str());
            }
            EXPECT_EQ("abc", s);
        }
        {
            std::unique_ptr<std::string> s(new std::string);
            abel::strings_internal::OStringStream strm(s.get());
            s.reset();
        }
    }

    TEST(OStringStream, Str) {
        std::string s1;
        abel::strings_internal::OStringStream strm(&s1);
        const abel::strings_internal::OStringStream &c_strm(strm);

        static_assert(std::is_same<decltype(strm.str()), std::string *>(), "");
        static_assert(std::is_same<decltype(c_strm.str()), const std::string *>(), "");

        EXPECT_EQ(&s1, strm.str());
        EXPECT_EQ(&s1, c_strm.str());

        strm.str(&s1);
        EXPECT_EQ(&s1, strm.str());
        EXPECT_EQ(&s1, c_strm.str());

        std::string s2;
        strm.str(&s2);
        EXPECT_EQ(&s2, strm.str());
        EXPECT_EQ(&s2, c_strm.str());

        strm.str(nullptr);
        EXPECT_EQ(nullptr, strm.str());
        EXPECT_EQ(nullptr, c_strm.str());
    }

    TEST(OStreamStream, WriteToLValue) {
        std::string s = "abc";
        {
            abel::strings_internal::OStringStream strm(&s);
            EXPECT_EQ("abc", s);
            strm << "";
            EXPECT_EQ("abc", s);
            strm << 42;
            EXPECT_EQ("abc42", s);
            strm << 'x' << 'y';
            EXPECT_EQ("abc42xy", s);
        }
        EXPECT_EQ("abc42xy", s);
    }

    TEST(OStreamStream, WriteToRValue) {
        std::string s = "abc";
        abel::strings_internal::OStringStream(&s) << "";
        EXPECT_EQ("abc", s);
        abel::strings_internal::OStringStream(&s) << 42;
        EXPECT_EQ("abc42", s);
        abel::strings_internal::OStringStream(&s) << 'x' << 'y';
        EXPECT_EQ("abc42xy", s);
    }

}  // namespace
