

#ifndef ABEL_FORMAT_INTERNAL_LENGTH_MOD_H_
#define ABEL_FORMAT_INTERNAL_LENGTH_MOD_H_
#include <abel/strings/string_view.h>
#include <ostream>
#include <string>

namespace abel {
namespace format_internal {



struct length_mod {
public:
    enum Id : uint8_t {
        h, hh, l, ll, L, j, z, t, q, none
    };
    static const size_t kNumValues = none + 1;

    length_mod () : id_(none) { }

    // Index into the opaque array of LengthMod enums.
    // Requires: i < kNumValues
    static length_mod FromIndex (size_t i) {
        return length_mod(kSpecs[i].value);
    }

    static length_mod FromId (Id id) { return length_mod(id); }

    // The length modifier std::string associated with a specified LengthMod.
    string_view name () const {
        const Spec &spec = kSpecs[id_];
        return {spec.name, spec.name_length};
    }

    Id id () const { return id_; }

    friend bool operator == (const length_mod &a, const length_mod &b) {
        return a.id() == b.id();
    }
    friend bool operator != (const length_mod &a, const length_mod &b) {
        return !(a == b);
    }
    friend std::ostream &operator << (std::ostream &os, const length_mod &v) {
        return os << v.name();
    }

private:
    struct Spec {
        Id value;
        const char *name;
        size_t name_length;
    };
    static const Spec kSpecs[];

    explicit length_mod (Id id) : id_(id) { }

    Id id_;
};


} //namespace format_internal
} //namespace abel
#endif //ABEL_FORMAT_INTERNAL_LENGTH_MOD_H_
