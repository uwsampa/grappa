/* Copyright (C) 2010 The Trustees of Indiana University.                  */
/*                                                                         */
/* Use, modification and distribution is subject to the Boost Software     */
/* License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at */
/* http://www.boost.org/LICENSE_1_0.txt)                                   */
/*                                                                         */
/*  Authors: Jeremiah Willcock                                             */
/*           Andrew Lumsdaine                                              */

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <assert.h>
#include "common.h"

void* xrealloc(void* p, size_t nbytes) {
  p = realloc(p, nbytes);
  if (!p && nbytes != 0) {
    fprintf(stderr, "realloc() failed for size %zu\n", nbytes);
    abort();
  }
  return p;
}
