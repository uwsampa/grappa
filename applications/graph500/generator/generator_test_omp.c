/* Copyright (C) 2009-2010 The Trustees of Indiana University.             */
/*                                                                         */
/* Use, modification and distribution is subject to the Boost Software     */
/* License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at */
/* http://www.boost.org/LICENSE_1_0.txt)                                   */
/*                                                                         */
/*  Authors: Jeremiah Willcock                                             */
/*           Andrew Lumsdaine                                              */

#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>
#include <stdio.h>
#include <omp.h>

#include "make_graph.h"

int main(int argc, char* argv[]) {
	int log_numverts;
	double start, time_taken;
	int64_t nedges, actual_nedges;
	packed_edge* result;
	
	log_numverts = 16; /* In base 2 */
	if (argc >= 2) log_numverts = atoi(argv[1]);
	
	/* Start of graph generation timing */
	start = omp_get_wtime();
	
	make_graph(log_numverts, INT64_C(16) << log_numverts, 1, 2, &nedges, &result);
	
	time_taken = omp_get_wtime() - start;
	/* End of graph generation timing */
	
	actual_nedges = nedges;
	
	fprintf(stderr, "%" PRIu64 " edge%s generated in %fs (%f Medges/s)\n", actual_nedges, (actual_nedges == 1 ? "" : "s"), time_taken, 1. * actual_nedges / time_taken * 1.e-6);
	
	free(result);
	
	return 0;
}
