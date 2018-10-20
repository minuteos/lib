/*
 * Copyright (c) 2018 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * macros.h - unified macros across compilers
 */

#if __GNUC__
#define ALWAYS_INLINE   __attribute__((always_inline))
#define NO_INLINE       __attribute__((noinline))
#else
#define ALWAYS_INLINE   inline
#define NO_INLINE
#endif
