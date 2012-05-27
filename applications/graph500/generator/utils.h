/* Copyright (C) 2010 The Trustees of Indiana University.                  */
/*                                                                         */
/* Use, modification and distribution is subject to the Boost Software     */
/* License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at */
/* http://www.boost.org/LICENSE_1_0.txt)                                   */
/*                                                                         */
/*  Authors: Jeremiah Willcock                                             */
/*           Andrew Lumsdaine                                              */

#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdint.h>
#include "splittable_mrg.h"

#ifdef __cplusplus
extern "C" {
#endif

uint_fast64_t random_up_to(mrg_state* st, uint_fast64_t n);
void make_mrg_seed(uint64_t userseed1, uint64_t userseed2, uint_fast32_t* seed);

#ifdef __cplusplus
}
#endif

#endif /* UTILS_H */
