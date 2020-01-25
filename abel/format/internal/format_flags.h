
#ifndef ABEL_FORMAT_INTERNAL_FORMAT_FLAGS_H_
#define ABEL_FORMAT_INTERNAL_FORMAT_FLAGS_H_
#include <ostream>
#include <string>

namespace abel {
namespace format_internal {


struct format_flags {
    bool basic : 1;     // fastest conversion: no flags, width, or precision
    bool left : 1;      // "-"
    bool show_pos : 1;  // "+"
    bool sign_col : 1;  // " "
    bool alt : 1;       // "#"
    bool zero : 1;      // "0"
    std::string to_string () const;
    friend std::ostream &operator << (std::ostream &os, const format_flags &v) {
        return os << v.to_string();
    }
};

}  //namespace format_internal
} //namespace abel
#endif //ABEL_FORMAT_INTERNAL_FORMAT_FLAGS_H_
