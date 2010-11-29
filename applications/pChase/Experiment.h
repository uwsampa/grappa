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


#if !defined(Experiment_h)
#define Experiment_h

#include "Chain.h"
#include "Types.h"

class Experiment {
public:
    Experiment();
    ~Experiment();

    int   parse_args(int argc, char* argv[]); 
    int64 parse_number( const char* s );
    float parse_real( const char* s );

    const char* placement();
    const char* access();

			    // fundamental parameters
    int64 pointer_size;		// number of bytes in a pointer
    int64 bytes_per_line;	// working set cache line size (bytes)
    int64 links_per_line;	// working set cache line size (links)
    int64 bytes_per_page;	// working set page size (in bytes)
    int64 lines_per_page;	// working set page size (in lines)
    int64 links_per_page;	// working set page size (in links)
    int64 bytes_per_chain;	// working set chain size (bytes)
    int64 lines_per_chain;	// working set chain size (lines)
    int64 links_per_chain;	// working set chain size (links)
    int64 pages_per_chain;	// working set chain size (pages)
    int64 bytes_per_thread;	// thread working set size (bytes)
    int64 chains_per_thread;	// memory loading per thread
    int64 num_threads;		// number of threads in the experiment
    int64 bytes_per_test;	// test working set size (bytes)

    float seconds;		// number of seconds per experiment
    int64 iterations;		// number of iterations per experiment
    int64 experiments;		// number of experiments per test

    enum { CSV, BOTH, HEADER, TABLE }
	output_mode;		// results output mode

    enum { RANDOM, STRIDED, STREAM }
	access_pattern;		// memory access pattern
    int64 stride;

    enum { LOCAL, XOR, ADD, MAP }
	numa_placement;		// memory allocation mode
    int64 offset_or_mask;
    char* placement_map;

				// maps threads and chains to numa domains
    int32* thread_domain;	// thread_domain[thread]
    int32** chain_domain;	// chain_domain[thread][chain]
    int32 numa_max_domain;	// highest numa domain id
    int32 num_numa_domains;	// number of numa domains

    char** random_state;	// random state for each thread

    int strict;			// strictly adhere to user input, or fail

    const static int32 DEFAULT_POINTER_SIZE      = sizeof(Chain);
    const static int32 DEFAULT_BYTES_PER_LINE    = 64;
    const static int32 DEFAULT_LINKS_PER_LINE    = DEFAULT_BYTES_PER_LINE / DEFAULT_POINTER_SIZE;
    const static int32 DEFAULT_BYTES_PER_PAGE    = 4096;
    const static int32 DEFAULT_LINES_PER_PAGE    = DEFAULT_BYTES_PER_PAGE / DEFAULT_BYTES_PER_LINE;
    const static int32 DEFAULT_LINKS_PER_PAGE    = DEFAULT_LINES_PER_PAGE * DEFAULT_LINKS_PER_LINE;
    const static int32 DEFAULT_PAGES_PER_CHAIN   = 4096;
    const static int32 DEFAULT_BYTES_PER_CHAIN   = DEFAULT_BYTES_PER_PAGE * DEFAULT_PAGES_PER_CHAIN;
    const static int32 DEFAULT_LINES_PER_CHAIN   = DEFAULT_LINES_PER_PAGE * DEFAULT_PAGES_PER_CHAIN;
    const static int32 DEFAULT_LINKS_PER_CHAIN   = DEFAULT_LINES_PER_CHAIN * DEFAULT_BYTES_PER_LINE / DEFAULT_POINTER_SIZE;
    const static int32 DEFAULT_CHAINS_PER_THREAD = 1;
    const static int32 DEFAULT_BYTES_PER_THREAD  = DEFAULT_BYTES_PER_CHAIN * DEFAULT_CHAINS_PER_THREAD;
    const static int32 DEFAULT_THREADS           = 1;
    const static int32 DEFAULT_BYTES_PER_TEST    = DEFAULT_BYTES_PER_THREAD * DEFAULT_THREADS;
    const static int32 DEFAULT_SECONDS           = 1;
    const static int32 DEFAULT_ITERATIONS        = 0;
    const static int32 DEFAULT_EXPERIMENTS       = 1;

    const static int32 DEFAULT_OUTPUT_MODE       = 1;

    void alloc_local();
    void alloc_xor();
    void alloc_add();
    void alloc_map();

    void print();

private:
};

#endif
