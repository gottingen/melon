
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************///

#include "melon/debugging/symbolize.h"

#if defined(MELON_INTERNAL_HAVE_ELF_SYMBOLIZE)
#include "melon/debugging/symbolize_elf.h"
#elif defined(_WIN32)
// The Windows Symbolizer only works if PDB files containing the debug info
// are available to the program at runtime.
#include "melon/debugging/symbolize_win32.inc"
#else

#include "melon/debugging/symbolize_unimplemented.inc"

#endif
