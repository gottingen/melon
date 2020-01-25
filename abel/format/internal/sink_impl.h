

#ifndef ABEL_FORMAT_INTERNAL_SINK_IMPL_H_
#define ABEL_FORMAT_INTERNAL_SINK_IMPL_H_
#include <abel/strings/string_view.h>
#include <abel/format/internal/output.h>

namespace abel {

namespace format_internal {

class format_raw_sink_impl {
public:
    // Implicitly convert from any type that provides the hook function as
    // described above.
    template<typename T, decltype(format_internal::invoke_flush(
        std::declval<T *>(), string_view())) * = nullptr>
    format_raw_sink_impl (T *raw)  // NOLINT
        : sink_(raw), write_(&format_raw_sink_impl::Flush<T>) { }

    void Write (string_view s) { write_(sink_, s); }

    template<typename T>
    static format_raw_sink_impl Extract (T s) {
        return s._sink;
    }

private:
    template<typename T>
    static void Flush (void *r, string_view s) {
        format_internal::invoke_flush(static_cast<T *>(r), s);
    }

    void *sink_;
    void (*write_) (void *, string_view);
};

// An abstraction to which conversions write their string data.
class format_sink_impl {
public:
    explicit format_sink_impl (format_raw_sink_impl raw) : raw_(raw) { }

    ~format_sink_impl () { Flush(); }

    void Flush () {
        raw_.Write(string_view(buf_, pos_ - buf_));
        pos_ = buf_;
    }

    void Append (size_t n, char c) {
        if (n == 0)
            return;
        size_ += n;
        auto raw_append = [&] (size_t count) {
            memset(pos_, c, count);
            pos_ += count;
        };
        while (n > Avail()) {
            n -= Avail();
            if (Avail() > 0) {
                raw_append(Avail());
            }
            Flush();
        }
        raw_append(n);
    }

    void Append (string_view v) {
        size_t n = v.size();
        if (n == 0)
            return;
        size_ += n;
        if (n >= Avail()) {
            Flush();
            raw_.Write(v);
            return;
        }
        memcpy(pos_, v.data(), n);
        pos_ += n;
    }

    size_t size () const { return size_; }

    // Put 'v' to 'sink' with specified width, precision, and left flag.
    bool PutPaddedString (string_view v, int w, int p, bool l);

    template<typename T>
    T Wrap () {
        return T(this);
    }

    template<typename T>
    static format_sink_impl *Extract (T *s) {
        return s->sink_;
    }

private:
    size_t Avail () const { return buf_ + sizeof(buf_) - pos_; }

    format_raw_sink_impl raw_;
    size_t size_ = 0;
    char *pos_ = buf_;
    char buf_[1024];
};

// Return capacity - used, clipped to a minimum of 0.
ABEL_FORCE_INLINE size_t Excess (size_t used, size_t capacity) {
    return used < capacity ? capacity - used : 0;
}

} //namespace format_internal
} //namespace abel
#endif //ABEL_FORMAT_INTERNAL_SINK_IMPL_H_
