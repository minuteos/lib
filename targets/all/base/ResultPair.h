/*
 * Copyright (c) 2019 triaxis s.r.o.
 * Licensed under the MIT license. See LICENSE.txt file in the repository root
 * for full license information.
 *
 * ResultPair.h
 *
 * Efficient implementation of returning two values from a function
 */

#pragma once

#include <base/base.h>

#if __ARM_PCS || __ARM_PCS_VFP

// ARM PCS returns vectors in general purpose registers, we can exploit this to return results in R0/R1
// however, we must force the calling convention to AAPCS, otherwise we'd get the values in FP registers with VFP

typedef intptr_t __attribute__((vector_size(sizeof(intptr_t) * 2))) res_pair_t;

#define RES_PAIR_DECL_ATTRIBUTE         __attribute__((pcs("aapcs")))
#define RES_PAIR_DECL(fn, ...)          RES_PAIR_DECL_ATTRIBUTE res_pair_t fn(__VA_ARGS__)
#define RES_PAIR_DECL_EX(fn, ex, ...)   RES_PAIR_DECL_ATTRIBUTE res_pair_t fn(__VA_ARGS__) ex
#define RES_PAIR_FIRST(res) ((res)[0])
#define RES_PAIR_SECOND(res) ((res)[1])
#define RES_PAIR(first, second) ((res_pair_t){ (intptr_t)(first), (intptr_t)(second) })

#else

// use a simple structure otherwise
// this is just fine for e.g. Intel where structures that fit in [RE]AX:[RE]DX
// are returned as a register pair automatically
typedef struct { intptr_t first, second; } res_pair_t;

#define RES_PAIR_DECL_ATTRIBUTE
#define RES_PAIR_DECL(fn, ...)  res_pair_t fn(__VA_ARGS__)
#define RES_PAIR_DECL_EX(fn, ex, ...)  res_pair_t fn(__VA_ARGS__) ex
#define RES_PAIR_FIRST(res) ((res).first)
#define RES_PAIR_SECOND(res) ((res).second)
#define RES_PAIR(first, second) ((res_pair_t){ (intptr_t)(first), (intptr_t)(second) })

#endif
