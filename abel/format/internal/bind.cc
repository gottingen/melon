

#include <abel/format/internal/bind.h>
#include <cerrno>
#include <limits>
#include <sstream>
#include <string>

namespace abel {

namespace format_internal {

namespace {

ABEL_FORCE_INLINE bool BindFromPosition (int position, int *value,
                                         abel::Span<const format_arg_impl> pack) {
    assert(position > 0);
    if (static_cast<size_t>(position) > pack.size()) {
        return false;
    }
    // -1 because positions are 1-based
    return format_arg_impl_friend::to_int(pack[position - 1], value);
}

class ArgContext {
public:
    explicit ArgContext (abel::Span<const format_arg_impl> pack) : pack_(pack) { }

    // Fill 'bound' with the results of applying the context's argument pack
    // to the specified 'unbound'. We synthesize a bound_conversion by
    // lining up a unbound_conversion with a user argument. We also
    // resolve any '*' specifiers for width and precision, so after
    // this call, 'bound' has all the information it needs to be formatted.
    // Returns false on failure.
    bool Bind (const unbound_conversion *unbound, bound_conversion *bound);

private:
    abel::Span<const format_arg_impl> pack_;
};

ABEL_FORCE_INLINE bool ArgContext::Bind (const unbound_conversion *unbound,
                                         bound_conversion *bound) {
    const format_arg_impl *arg = nullptr;
    int arg_position = unbound->arg_position;
    if (static_cast<size_t>(arg_position - 1) >= pack_.size())
        return false;
    arg = &pack_[arg_position - 1];  // 1-based

    if (!unbound->flags.basic) {
        int width = unbound->width.value();
        bool force_left = false;
        if (unbound->width.is_from_arg()) {
            if (!BindFromPosition(unbound->width.get_from_arg(), &width, pack_))
                return false;
            if (width < 0) {
                // "A negative field width is taken as a '-' flag followed by a
                // positive field width."
                force_left = true;
                // Make sure we don't overflow the width when negating it.
                width = -std::max(width, -std::numeric_limits<int>::max());
            }
        }

        int precision = unbound->precision.value();
        if (unbound->precision.is_from_arg()) {
            if (!BindFromPosition(unbound->precision.get_from_arg(), &precision,
                                  pack_))
                return false;
        }

        bound->set_width(width);
        bound->set_precision(precision);
        bound->set_flags(unbound->flags);
        if (force_left)
            bound->set_left(true);
    } else {
        bound->set_flags(unbound->flags);
        bound->set_width(-1);
        bound->set_precision(-1);
    }

    bound->set_length_mod(unbound->length_mod);
    bound->set_conv(unbound->conv);
    bound->set_arg(arg);
    return true;
}

template<typename Converter>
class ConverterConsumer {
public:
    ConverterConsumer (Converter converter, abel::Span<const format_arg_impl> pack)
        : converter_(converter), arg_context_(pack) { }

    bool Append (string_view s) {
        converter_.Append(s);
        return true;
    }
    bool ConvertOne (const unbound_conversion &conv, string_view conv_string) {
        bound_conversion bound;
        if (!arg_context_.Bind(&conv, &bound))
            return false;
        return converter_.ConvertOne(bound, conv_string);
    }

private:
    Converter converter_;
    ArgContext arg_context_;
};

template<typename Converter>
bool ConvertAll (const untyped_format_spec_impl format,
                 abel::Span<const format_arg_impl> args, Converter converter) {
    if (format.has_parsed_conversion()) {
        return format.parsed_conversion()->process_format(
            ConverterConsumer<Converter>(converter, args));
    } else {
        return parse_format_string(format.str(),
                                   ConverterConsumer<Converter>(converter, args));
    }
}

class DefaultConverter {
public:
    explicit DefaultConverter (format_sink_impl *sink) : sink_(sink) { }

    void Append (string_view s) const { sink_->Append(s); }

    bool ConvertOne (const bound_conversion &bound, string_view /*conv*/) const {
        return format_arg_impl_friend::convert(*bound.arg(), bound, sink_);
    }

private:
    format_sink_impl *sink_;
};

class SummarizingConverter {
public:
    explicit SummarizingConverter (format_sink_impl *sink) : sink_(sink) { }

    void Append (string_view s) const { sink_->Append(s); }

    bool ConvertOne (const bound_conversion &bound, string_view /*conv*/) const {
        untyped_format_spec_impl spec("%d");

        std::ostringstream ss;
        ss << "{" << stream_able(spec, {*bound.arg()}) << ":" << bound.flags();
        if (bound.width() >= 0)
            ss << bound.width();
        if (bound.precision() >= 0)
            ss << "." << bound.precision();
        ss << bound.length_mod() << bound.conv() << "}";
        Append(ss.str());
        return true;
    }

private:
    format_sink_impl *sink_;
};

}  // namespace

bool BindWithPack (const unbound_conversion *props,
                   abel::Span<const format_arg_impl> pack,
                   bound_conversion *bound) {
    return ArgContext(pack).Bind(props, bound);
}

std::string Summarize (const untyped_format_spec_impl format,
                       abel::Span<const format_arg_impl> args) {
    typedef SummarizingConverter Converter;
    std::string out;
    {
        // inner block to destroy sink before returning out. It ensures a last
        // flush.
        format_sink_impl sink(&out);
        if (!ConvertAll(format, args, Converter(&sink))) {
            return "";
        }
    }
    return out;
}

bool FormatUntyped (format_raw_sink_impl raw_sink,
                    const untyped_format_spec_impl format,
                    abel::Span<const format_arg_impl> args) {
    format_sink_impl sink(raw_sink);
    using Converter = DefaultConverter;
    return ConvertAll(format, args, Converter(&sink));
}

std::ostream &stream_able::Print (std::ostream &os) const {
    if (!FormatUntyped(&os, format_, args_))
        os.setstate(std::ios::failbit);
    return os;
}

std::string &AppendPack (std::string *out, const untyped_format_spec_impl format,
                         abel::Span<const format_arg_impl> args) {
    size_t orig = out->size();
    if (ABEL_UNLIKELY(!FormatUntyped(out, format, args))) {
        out->erase(orig);
    }
    return *out;
}

std::string FormatPack (const untyped_format_spec_impl format,
                        abel::Span<const format_arg_impl> args) {
    std::string out;
    if (ABEL_UNLIKELY(!FormatUntyped(&out, format, args))) {
        out.clear();
    }
    return out;
}

int FprintF (std::FILE *output, const untyped_format_spec_impl format,
             abel::Span<const format_arg_impl> args) {
    file_raw_sink sink(output);
    if (!FormatUntyped(&sink, format, args)) {
        errno = EINVAL;
        return -1;
    }
    if (sink.error()) {
        errno = sink.error();
        return -1;
    }
    if (sink.count() > std::numeric_limits<int>::max()) {
        errno = EFBIG;
        return -1;
    }
    return static_cast<int>(sink.count());
}

int SnprintF (char *output, size_t size, const untyped_format_spec_impl format,
              abel::Span<const format_arg_impl> args) {
    buffer_raw_sink sink(output, size ? size - 1 : 0);
    if (!FormatUntyped(&sink, format, args)) {
        errno = EINVAL;
        return -1;
    }
    size_t total = sink.total_written();
    if (size)
        output[std::min(total, size - 1)] = 0;
    return static_cast<int>(total);
}

}  // namespace format_internal

}  // namespace abel
