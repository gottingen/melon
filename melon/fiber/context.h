//
// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
//


#ifndef MELON_FIBER_CONTEXT_H_
#define MELON_FIBER_CONTEXT_H_

#include <stdint.h>
#include <stdio.h>
#include <stddef.h>

#if defined(__GNUC__) || defined(__APPLE__)

  #define FIBER_CONTEXT_COMPILER_gcc

  #if defined(__linux__)
	#ifdef __x86_64__
	    #define FIBER_CONTEXT_PLATFORM_linux_x86_64
	    #define FIBER_CONTEXT_CALL_CONVENTION

	#elif __i386__
	    #define FIBER_CONTEXT_PLATFORM_linux_i386
	    #define FIBER_CONTEXT_CALL_CONVENTION
	#elif __arm__
	    #define FIBER_CONTEXT_PLATFORM_linux_arm32
	    #define FIBER_CONTEXT_CALL_CONVENTION
	#elif __aarch64__
	    #define FIBER_CONTEXT_PLATFORM_linux_arm64
	    #define FIBER_CONTEXT_CALL_CONVENTION
        #elif __loongarch64
            #define FIBER_CONTEXT_PLATFORM_linux_loongarch64
            #define FIBER_CONTEXT_CALL_CONVENTION
	#endif

  #elif defined(__MINGW32__) || defined (__MINGW64__)
	#if defined(__x86_64__)
	    #define FIBER_CONTEXT_COMPILER_gcc
    	    #define FIBER_CONTEXT_PLATFORM_windows_x86_64
	    #define FIBER_CONTEXT_CALL_CONVENTION
	#elif defined(__i386__)
	    #define FIBER_CONTEXT_COMPILER_gcc
	    #define FIBER_CONTEXT_PLATFORM_windows_i386
	    #define FIBER_CONTEXT_CALL_CONVENTION __cdecl
	#endif

  #elif defined(__APPLE__) && defined(__MACH__)
	#if defined (__i386__)
	    #define FIBER_CONTEXT_PLATFORM_apple_i386
	    #define FIBER_CONTEXT_CALL_CONVENTION
	#elif defined (__x86_64__)
	    #define FIBER_CONTEXT_PLATFORM_apple_x86_64
	    #define FIBER_CONTEXT_CALL_CONVENTION
	#elif defined (__aarch64__)
	    #define FIBER_CONTEXT_PLATFORM_apple_arm64
	    #define FIBER_CONTEXT_CALL_CONVENTION
    #endif
  #endif

#endif

#if defined(_WIN32_WCE)
typedef int intptr_t;
#endif

typedef void* fiber_fcontext_t;

#ifdef __cplusplus
extern "C"{
#endif

intptr_t FIBER_CONTEXT_CALL_CONVENTION
fiber_jump_fcontext(fiber_fcontext_t * ofc, fiber_fcontext_t nfc,
                      intptr_t vp, bool preserve_fpu = false);
fiber_fcontext_t FIBER_CONTEXT_CALL_CONVENTION
fiber_make_fcontext(void* sp, size_t size, void (* fn)( intptr_t));

#ifdef __cplusplus
};
#endif

#endif  // MELON_FIBER_CONTEXT_H_
