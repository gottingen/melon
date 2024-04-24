// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MUTIL_BASE_EXPORT_H_
#define MUTIL_BASE_EXPORT_H_

#if defined(COMPONENT_BUILD)
#if defined(WIN32)

#if defined(MUTIL_IMPLEMENTATION)
#define MUTIL_EXPORT __declspec(dllexport)
#define MUTIL_EXPORT_PRIVATE __declspec(dllexport)
#else
#define MUTIL_EXPORT __declspec(dllimport)
#define MUTIL_EXPORT_PRIVATE __declspec(dllimport)
#endif  // defined(MUTIL_IMPLEMENTATION)

#else  // defined(WIN32)
#if defined(MUTIL_IMPLEMENTATION)
#define MUTIL_EXPORT __attribute__((visibility("default")))
#define MUTIL_EXPORT_PRIVATE __attribute__((visibility("default")))
#else
#define MUTIL_EXPORT
#define MUTIL_EXPORT_PRIVATE
#endif  // defined(MUTIL_IMPLEMENTATION)
#endif

#else  // defined(COMPONENT_BUILD)
#define MUTIL_EXPORT
#define MUTIL_EXPORT_PRIVATE
#endif

#endif  // MUTIL_BASE_EXPORT_H_
