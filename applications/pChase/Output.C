/*******************************************************************************
 * Copyright (c) 2006 International Business Machines Corporation.             *
 * All rights reserved. This program and the accompanying materials            *
 * are made available under the terms of the Common Public License v1.0        *
 * which accompanies this distribution, and is available at                    *
 * http://www.opensource.org/licenses/cpl1.0.php                               *
 *                                                                             *
 * Contributors:                                                               *
 *    Douglas M. Pase - initial API and implementation                         *
 *******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Output.h"

#include "Types.h"
#include "Experiment.h"


void
Output::print( Experiment &e, int64 ops, double secs, double ck_res )
{
    if (e.output_mode == Experiment::CSV) {
	Output::csv(e, ops, secs, ck_res);
    } else if (e.output_mode == Experiment::BOTH) {
	Output::header(e, ops, secs, ck_res);
	Output::csv(e, ops, secs, ck_res);
    } else if (e.output_mode == Experiment::HEADER) {
	Output::header(e, ops, secs, ck_res);
    } else {
	Output::table(e, ops, secs, ck_res);
    }
}

void
Output::header( Experiment &e, int64 ops, double secs, double ck_res )
{
    printf("pointer size (bytes),");
    printf("cache line size (bytes),");
    printf("page size (bytes),");
    printf("chain size (bytes),");
    printf("thread size (bytes),");
    printf("test size (bytes),");
    printf("chains per thread,");
    printf("number of threads,");
    printf("iterations,");
    printf("experiments,");
    printf("access pattern,");
    printf("stride,");
    printf("numa placement,");
    printf("offset or mask,");
    printf("numa domains,");
    printf("domain map,");
    printf("operations per chain,");
    printf("total operations,");
    printf("elapsed time (seconds),");
    printf("elapsed time (timer ticks),");
    printf("clock resolution (ns),", ck_res * 1E9);
    printf("memory latency (ns),");
    printf("memory bandwidth (MB/s)\n");

    fflush(stdout);
}

void
Output::csv( Experiment &e, int64 ops, double secs, double ck_res )
{
    printf("%ld,", e.pointer_size);
    printf("%ld,", e.bytes_per_line);
    printf("%ld,", e.bytes_per_page);
    printf("%ld,", e.bytes_per_chain);
    printf("%ld,", e.bytes_per_thread);
    printf("%ld,", e.bytes_per_test);
    printf("%lld,", e.chains_per_thread);
    printf("%ld,", e.num_threads);
    printf("%ld,", e.iterations);
    printf("%ld,", e.experiments);
    printf("%s,", e.access());
    printf("%ld,", e.stride);
    printf("%s,", e.placement());
    printf("%ld,", e.offset_or_mask);
    printf("%ld,", e.num_numa_domains);
    printf("\"");
    printf("%d:", e.thread_domain[0]);
    printf("%d", e.chain_domain[0][0]);
    for (int j=1; j < e.chains_per_thread; j++) {
	printf(",%d", e.chain_domain[0][j]);
    }
    for (int i=1; i < e.num_threads; i++) {
	printf(";%d:", e.thread_domain[i]);
	printf("%d", e.chain_domain[i][0]);
	for (int j=1; j < e.chains_per_thread; j++) {
	    printf(",%d", e.chain_domain[i][j]);
	}
    }
    printf("\",");
    printf("%ld,", ops);
    printf("%ld,", ops * e.chains_per_thread * e.num_threads);
    printf("%.3f,", secs);
    printf("%.0f,", secs/ck_res);
    printf("%.2f,", ck_res * 1E9);
    printf("%.2f,", (secs / (ops * e.iterations)) * 1E9);
    printf("%.3f\n", ((ops * e.iterations * e.chains_per_thread * e.num_threads * e.bytes_per_line) / secs) * 1E-6);

    fflush(stdout);
}

void
Output::table( Experiment &e, int64 ops, double secs, double ck_res )
{
    printf("pointer size         = %ld (bytes)\n", e.pointer_size);
    printf("cache line size      = %ld (bytes)\n", e.bytes_per_line);
    printf("page size            = %ld (bytes)\n", e.bytes_per_page);
    printf("chain size           = %ld (bytes)\n", e.bytes_per_chain);
    printf("thread size          = %ld (bytes)\n", e.bytes_per_thread);
    printf("test size            = %ld (bytes)\n", e.bytes_per_test);
    printf("chains per thread    = %ld\n", e.chains_per_thread);
    printf("number of threads    = %ld\n", e.num_threads);
    printf("iterations           = %ld\n", e.iterations);
    printf("experiments          = %ld\n", e.experiments);
    printf("access pattern       = %s\n", e.access());
    printf("stride               = %ld\n", e.stride);
    printf("numa placement       = %s\n", e.placement());
    printf("offset or mask       = %ld\n", e.offset_or_mask);
    printf("numa domains         = %ld\n", e.num_numa_domains);
    printf("domain map           = ");
    printf("\"");
    printf("%d:", e.thread_domain[0]);
    printf("%d", e.chain_domain[0][0]);
    for (int j=1; j < e.chains_per_thread; j++) {
	printf(",%d", e.chain_domain[0][j]);
    }
    for (int i=1; i < e.num_threads; i++) {
	printf(";%d:", e.thread_domain[i]);
	printf("%d", e.chain_domain[i][0]);
	for (int j=1; j < e.chains_per_thread; j++) {
	    printf(",%d", e.chain_domain[i][j]);
	}
    }
    printf("\"\n");
    printf("operations per chain = %ld\n", ops);
    printf("total operations     = %ld\n", ops * e.chains_per_thread * e.num_threads);
    printf("elapsed time         = %.3f (seconds)\n", secs);
    printf("elapsed time         = %.0f (timer ticks)\n", secs/ck_res);
    printf("clock resolution     = %.2f (ns)\n", ck_res * 1E9);
    printf("memory latency       = %.2f (ns)\n", (secs / (ops * e.iterations)) * 1E9);
    printf("memory bandwidth     = %.3f (MB/s)\n", ((ops * e.iterations * e.chains_per_thread * e.num_threads * e.bytes_per_line) / secs) * 1E-6);

    fflush(stdout);
}
