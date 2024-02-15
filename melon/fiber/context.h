// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
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
