
#ifndef ABEL_FORMAT_INTERNAL_CONVERSION_CHAR_H_
#define ABEL_FORMAT_INTERNAL_CONVERSION_CHAR_H_
#include <cstdint>
#include <cstddef>
#include <iostream>

namespace abel {
namespace format_internal {


// clang-format off
#define ABEL_CONVERSION_CHARS_EXPAND_(X_VAL, X_SEP) \
  /* text */ \
  X_VAL(c) X_SEP X_VAL(C) X_SEP X_VAL(s) X_SEP X_VAL(S) X_SEP \
  /* ints */ \
  X_VAL(d) X_SEP X_VAL(i) X_SEP X_VAL(o) X_SEP \
  X_VAL(u) X_SEP X_VAL(x) X_SEP X_VAL(X) X_SEP \
  /* floats */ \
  X_VAL(f) X_SEP X_VAL(F) X_SEP X_VAL(e) X_SEP X_VAL(E) X_SEP \
  X_VAL(g) X_SEP X_VAL(G) X_SEP X_VAL(a) X_SEP X_VAL(A) X_SEP \
  /* misc */ \
  X_VAL(n) X_SEP X_VAL(p)
// clang-format on

struct conversion_char {
public:
    enum Id : uint8_t {
        c, C, s, S,              // text
        d, i, o, u, x, X,        // int
        f, F, e, E, g, G, a, A,  // float
        n, p,                    // misc
        none
    };
    static const size_t kNumValues = none + 1;

    conversion_char () : id_(none) { }

public:
    // Index into the opaque array of conversion_char enums.
    // Requires: i < kNumValues
    static conversion_char FromIndex (size_t i) {
        return conversion_char(kSpecs[i].value);
    }

    static conversion_char FromChar (char c) {
        conversion_char::Id out_id = conversion_char::none;
        switch (c) {
#define X_VAL(id)                \
  case #id[0]:                   \
    out_id = conversion_char::id; \
    break;
        ABEL_CONVERSION_CHARS_EXPAND_(X_VAL,)
#undef X_VAL
        default:break;
        }
        return conversion_char(out_id);
    }

    static conversion_char FromId (Id id) { return conversion_char(id); }
    Id id () const { return id_; }

    int radix () const {
        switch (id()) {
        case x:
        case X:
        case a:
        case A:
        case p: return 16;
        case o: return 8;
        default: return 10;
        }
    }

    bool upper () const {
        switch (id()) {
        case X:
        case F:
        case E:
        case G:
        case A: return true;
        default: return false;
        }
    }

    bool is_signed () const {
        switch (id()) {
        case d:
        case i: return true;
        default: return false;
        }
    }

    bool is_integral () const {
        switch (id()) {
        case d:
        case i:
        case u:
        case o:
        case x:
        case X:return true;
        default: return false;
        }
    }

    bool is_float () const {
        switch (id()) {
        case a:
        case e:
        case f:
        case g:
        case A:
        case E:
        case F:
        case G:return true;
        default: return false;
        }
    }

    bool IsValid () const { return id() != none; }

    // The associated char.
    char Char () const { return kSpecs[id_].name; }

    friend bool operator == (const conversion_char &a, const conversion_char &b) {
        return a.id() == b.id();
    }
    friend bool operator != (const conversion_char &a, const conversion_char &b) {
        return !(a == b);
    }
    friend std::ostream &operator << (std::ostream &os, const conversion_char &v) {
        char c = v.Char();
        if (!c)
            c = '?';
        return os << c;
    }

private:
    struct Spec {
        Id value;
        char name;
    };
    static const Spec kSpecs[];

    explicit conversion_char (Id id) : id_(id) { }

    Id id_;
};

} //namespace format_internal
} //namespace abel
#endif //ABEL_FORMAT_INTERNAL_CONVERSION_CHAR_H_
