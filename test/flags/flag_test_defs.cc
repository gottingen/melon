//

// This file is used to test the mismatch of the flag type between definition
// and declaration. These are definitions. flag_test.cc contains declarations.
#include <string>
#include <abel/flags/flag.h>

ABEL_FLAG(int, mistyped_int_flag, 0, "");
ABEL_FLAG(std::string, mistyped_string_flag, "", "");
ABEL_RETIRED_FLAG(bool, old_bool_flag, true,
                  "repetition of retired flag definition");
