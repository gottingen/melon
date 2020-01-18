//

#include <abel/debugging/symbolize.h>

#if defined(ABEL_INTERNAL_HAVE_ELF_SYMBOLIZE)
#include <abel/debugging/symbolize_elf.inc>
#elif defined(_WIN32)
// The Windows Symbolizer only works if PDB files containing the debug info
// are available to the program at runtime.
#include <abel/debugging/symbolize_win32.inc>
#else
#include <abel/debugging/symbolize_unimplemented.inc>
#endif
