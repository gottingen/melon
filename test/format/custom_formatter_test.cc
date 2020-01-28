//
// Created by liyinbin on 2020/1/26.
//
#include <abel/format/format.h>
#include <gtest/gtest.h>

class custom_arg_formatter :
    public fmt::arg_formatter<fmt::back_insert_range<fmt::internal::buffer>> {
public:
    typedef fmt::back_insert_range<fmt::internal::buffer> range;
    typedef fmt::arg_formatter<range> base;

    custom_arg_formatter (fmt::format_context &ctx, fmt::format_specs &s)
        : base(ctx, s) { }

    using base::operator ();

    iterator operator () (double value) {
        if (round(value * pow(10, spec().precision())) == 0)
            value = 0;
        return base::operator ()(value);
    }
};

std::string custom_vformat (abel::string_view format_str, fmt::format_args args) {
    fmt::memory_buffer buffer;
    // Pass custom argument formatter as a template arg to vwrite.
    fmt::vformat_to<custom_arg_formatter>(buffer, format_str, args);
    return std::string(buffer.data(), buffer.size());
}

template<typename... Args>
std::string custom_format (const char *format_str, const Args &... args) {
    auto va = fmt::make_format_args(args...);
    return custom_vformat(format_str, va);
}

TEST(CustomFormatterTest, Format) {
    EXPECT_EQ("0.00", custom_format("{:.2f}", -.00001));
}