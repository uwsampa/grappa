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
#include <unistd.h>

#if defined(NUMA)
#include <numa.h>
#endif

#include "Experiment.h"

Experiment::Experiment() :
    strict           (0),
    pointer_size     (DEFAULT_POINTER_SIZE),
    bytes_per_line   (DEFAULT_BYTES_PER_LINE),
    links_per_line   (DEFAULT_LINKS_PER_LINE),
    bytes_per_page   (DEFAULT_BYTES_PER_PAGE),
    lines_per_page   (DEFAULT_LINES_PER_PAGE),
    links_per_page   (DEFAULT_LINKS_PER_PAGE),
    bytes_per_chain  (DEFAULT_BYTES_PER_CHAIN),
    lines_per_chain  (DEFAULT_LINES_PER_CHAIN),
    links_per_chain  (DEFAULT_LINKS_PER_CHAIN),
    pages_per_chain  (DEFAULT_PAGES_PER_CHAIN),
    chains_per_thread(DEFAULT_CHAINS_PER_THREAD),
    bytes_per_thread (DEFAULT_BYTES_PER_THREAD),
    num_threads      (DEFAULT_THREADS), 
    bytes_per_test   (DEFAULT_BYTES_PER_TEST),
    seconds          (DEFAULT_SECONDS),
    iterations       (DEFAULT_ITERATIONS),
    experiments      (DEFAULT_EXPERIMENTS), 
    output_mode      (TABLE),
    access_pattern   (RANDOM),
    stride           (1),
    numa_placement   (LOCAL),
    offset_or_mask   (0),
    placement_map    (NULL),
    thread_domain    (NULL),
    chain_domain     (NULL),
    numa_max_domain  (0),
    num_numa_domains (1)
{
}

Experiment::~Experiment()
{
}

				// interface: 
				//
				// -l or --line             bytes per cache line (line size)
				// -p or --page             bytes per page  (page size)
				// -c or --chain            bytes per chain (used to compute pages per chain)
				// -r or --references       chains per thread (memory loading)
				// -t or --threads          number of threads (concurrency and contention)
				// -i or --iters            iterations
				// -e or --experiments      experiments
				// -a or --access           memory access pattern
				//         random           random access pattern
				//         forward <stride> exclusive OR and mask
				//         reverse <stride> addition and offset
				// -o or --output           output mode
				//         hdr              header only
				//         csv              csv only
				//         both             header + csv
				//         table            human-readable table of values
				// -n or --numa             numa placement
				//         local            local allocation of all chains
				//         xor <mask>       exclusive OR and mask
				//         add <offset>     addition and offset
				//         map <map>        explicit mapping of threads and chains to domains

int
Experiment::parse_args(int argc, char* argv[])
{
    int error = 0;
    for (int i=1; i < argc; i++) {
	if (strcasecmp(argv[i], "-x") == 0 || strcasecmp(argv[i], "--strict") == 0) {
	    this->strict = 1;
	} else if (strcasecmp(argv[i], "-s") == 0 || strcasecmp(argv[i], "--seconds") == 0) {
	    i++;
	    if (i == argc) { error = 1; break; }
	    this->seconds = Experiment::parse_real(argv[i]);
	    this->iterations = 0;
	    if (this->seconds == 0) { error = 1; break; }
	} else if (strcasecmp(argv[i], "-l") == 0 || strcasecmp(argv[i], "--line") == 0) {
	    i++;
	    if (i == argc) { error = 1; break; }
	    this->bytes_per_line = Experiment::parse_number(argv[i]);
	    if (this->bytes_per_line == 0) { error = 1; break; }
	} else if (strcasecmp(argv[i], "-p") == 0 || strcasecmp(argv[i], "--page") == 0) {
	    i++;
	    if (i == argc) { error = 1; break; }
	    this->bytes_per_page = Experiment::parse_number(argv[i]);
	    if (this->bytes_per_page == 0) { error = 1; break; }
	} else if (strcasecmp(argv[i], "-c") == 0 || strcasecmp(argv[i], "--chain") == 0) {
	    i++;
	    if (i == argc) { error = 1; break; }
	    this->bytes_per_chain = Experiment::parse_number(argv[i]);
	    if (this->bytes_per_chain == 0) { error = 1; break; }
	} else if (strcasecmp(argv[i], "-r") == 0 || strcasecmp(argv[i], "--references") == 0) {
	    i++;
	    if (i == argc) { error = 1; break; }
	    this->chains_per_thread = Experiment::parse_number(argv[i]);
	    if (this->chains_per_thread == 0) { error = 1; break; }
	} else if (strcasecmp(argv[i], "-t") == 0 || strcasecmp(argv[i], "--threads") == 0) {
	    i++;
	    if (i == argc) { error = 1; break; }
	    this->num_threads = Experiment::parse_number(argv[i]);
	    if (this->num_threads == 0) { error = 1; break; }
	} else if (strcasecmp(argv[i], "-i") == 0 || strcasecmp(argv[i], "--iterations") == 0) {
	    i++;
	    if (i == argc) { error = 1; break; }
	    this->iterations = Experiment::parse_number(argv[i]);
	    this->seconds = 0;
	    if (this->iterations == 0) { error = 1; break; }
	} else if (strcasecmp(argv[i], "-e") == 0 || strcasecmp(argv[i], "--experiments") == 0) {
	    i++;
	    if (i == argc) { error = 1; break; }
	    this->experiments = Experiment::parse_number(argv[i]);
	    if (this->experiments == 0) { error = 1; break; }
	} else if (strcasecmp(argv[i], "-a") == 0 || strcasecmp(argv[i], "--access") == 0) {
	    i++;
	    if (i == argc) { error = 1; break; }
	    if (strcasecmp(argv[i], "random") == 0) {
		this->access_pattern = RANDOM;
	    } else if (strcasecmp(argv[i], "forward") == 0) {
		this->access_pattern = STRIDED;
		i++;
		if (i == argc) { error = 1; break; }
		this->stride = Experiment::parse_number(argv[i]);
		if (this->stride == 0) { error = 1; break; }
	    } else if (strcasecmp(argv[i], "reverse") == 0) {
		this->access_pattern = STRIDED;
		i++;
		if (i == argc) { error = 1; break; }
		this->stride = - Experiment::parse_number(argv[i]);
		if (this->stride == 0) { error = 1; break; }
	    } else if (strcasecmp(argv[i], "stream") == 0) {
		this->access_pattern = STREAM;
		i++;
		if (i == argc) { error = 1; break; }
		this->stride = Experiment::parse_number(argv[i]);
		if (this->stride == 0) { error = 1; break; }
	    } else {
		error = 1;
		break;
	    }
	} else if (strcasecmp(argv[i], "-o") == 0 || strcasecmp(argv[i], "--output") == 0) {
	    i++;
	    if (i == argc) { error = 1; break; }
	    if (strcasecmp(argv[i], "table") == 0) {
		this->output_mode = TABLE;
	    } else if (strcasecmp(argv[i], "csv") == 0) {
		this->output_mode = CSV;
	    } else if (strcasecmp(argv[i], "both") == 0) {
		this->output_mode = BOTH;
	    } else if (strcasecmp(argv[i], "hdr") == 0) {
		this->output_mode = HEADER;
	    } else if (strcasecmp(argv[i], "header") == 0) {
		this->output_mode = HEADER;
	    } else {
		error = 1;
		break;
	    }
	} else if (strcasecmp(argv[i], "-n") == 0 || strcasecmp(argv[i], "--numa") == 0) {
	    i++;
	    if (i == argc) { error = 1; break; }
	    if (strcasecmp(argv[i], "local") == 0) {
		this->numa_placement = LOCAL;
	    } else if (strcasecmp(argv[i], "xor") == 0) {
		this->numa_placement = XOR;
		i++;
		if (i == argc) { error = 1; break; }
		this->offset_or_mask = Experiment::parse_number(argv[i]);
	    } else if (strcasecmp(argv[i], "add") == 0) {
		this->numa_placement = ADD;
		i++;
		if (i == argc) { error = 1; break; }
		this->offset_or_mask = Experiment::parse_number(argv[i]);
	    } else if (strcasecmp(argv[i], "map") == 0) {
		this->numa_placement = MAP;
		i++;
		if (i == argc) { error = 1; break; }
		this->placement_map = argv[i];
	    } else {
		error = 1;
		break;
	    }
	} else {
	    error = 1;
	    break;
	}
    }


				// if we've hit an error, print a message and quit
    if (error) {
	printf("usage: %s <options>\n", argv[0]);
	printf("where <options> are selected from the following:\n");
	printf("    [-h|--help]                    # this message\n");
	printf("    [-l|--line]        <number>    # bytes per cache line (cache line size)\n");
	printf("    [-p|--page]        <number>    # bytes per page (page size)\n");
	printf("    [-c|--chain]       <number>    # bytes per chain (used to compute pages per chain)\n");
	printf("    [-r|--references]  <number>    # chains per thread (memory loading)\n");
	printf("    [-t|--threads]     <number>    # number of threads (concurrency and contention)\n");
	printf("    [-i|--iterations]  <number>    # iterations per experiment\n");
	printf("    [-e|--experiments] <number>    # experiments\n");
	printf("    [-a|--access]      <pattern>   # memory access pattern\n");
	printf("    [-o|--output]      <format>    # output format\n");
	printf("    [-n|--numa]        <placement> # numa placement\n");
	printf("    [-s|--seconds]     <number>    # run each experiment for <number> seconds\n");
	printf("    [-x|--strict]                  # fail rather than adjust options to sensible values\n");
	printf("\n");
	printf("<pattern> is selected from the following:\n");
	printf("    random                         # all chains are accessed randomly\n");
	printf("    forward <stride>               # chains are in forward order with constant stride\n");
	printf("    reverse <stride>               # chains are in reverse order with constant stride\n");
	printf("    stream  <stride>               # references are calculated rather than read from memory\n");
	printf("\n");
	printf("Note: <stride> is always a small positive integer.\n");
	printf("\n");
	printf("<format> is selected from the following:\n");
	printf("    hdr                            # csv header only\n");
	printf("    csv                            # results in csv format only\n");
	printf("    both                           # header and results in csv format\n");
	printf("    table                          # human-readable table of values\n");
	printf("\n");
	printf("<placement> is selected from the following:\n");
	printf("    local                          # all chains are allocated locally\n");
	printf("    xor <mask>                     # exclusive OR and mask\n");
	printf("    add <offset>                   # addition and offset\n");
	printf("    map <map>                      # explicit mapping of threads and chains to domains\n");
	printf("\n");
	printf("<map> has the form \"t1:c11,c12,...,c1m;t2:c21,...,c2m;...;tn:cn1,...,cnm\"\n");
	printf("where t[i] is the NUMA domain where the ith thread is run,\n");
	printf("and c[i][j] is the NUMA domain where the jth chain in the ith thread is allocated.\n");
	printf("(The values t[i] and c[i][j] must all be zero or small positive integers.)\n");
	printf("\n");
	printf("Note: for maps, each thread must have the same number of chains,\n");
	printf("maps override the -t or --threads specification,\n");
	printf("NUMA domains are whole numbers in the range of 0..N, and\n");
	printf("thread or chain domains that exceed the maximum NUMA domain\n");
	printf("are wrapped around using a MOD function.\n");
	printf("\n");
	printf("To determine the number of NUMA domains currently available\n");
	printf("on your system, use a command such as \"numastat\".\n");
	printf("\n");
	printf("Final note: strict is not yet fully implemented, and\n");
	printf("maps do not gracefully handle ill-formed map specifications.\n");

	return 1;
    }


				// STRICT -- fail if specifications are inconsistent

				// compute lines per page and lines per chain
				// based on input and defaults.
				// we round up page and chain sizes when needed.
    this->lines_per_page   = (this->bytes_per_page+this->bytes_per_line-1) / this->bytes_per_line;
    this->bytes_per_page   = this->bytes_per_line * this->lines_per_page;
    this->pages_per_chain  = (this->bytes_per_chain+this->bytes_per_page-1) / this->bytes_per_page;
    this->bytes_per_chain  = this->bytes_per_page * this->pages_per_chain;
    this->bytes_per_thread = this->bytes_per_chain * this->chains_per_thread;
    this->bytes_per_test   = this->bytes_per_thread * this->num_threads;
    this->links_per_line   = this->bytes_per_line / pointer_size;
    this->links_per_page   = this->lines_per_page * this->links_per_line;
    this->lines_per_chain  = this->lines_per_page * this->pages_per_chain;
    this->links_per_chain  = this->lines_per_chain * this->links_per_line;


				// allocate the chain roots for all threads
				// and compute the chain locations
				// (the chains themselves are initialized by the threads)
    switch (this->numa_placement) {
    case LOCAL :
    case XOR :
    case ADD :
	this->thread_domain = new int32 [ this->num_threads ];
	this->chain_domain  = new int32*[ this->num_threads ];
	this->random_state  = new char* [ this->num_threads ];

	for (int i=0; i < this->num_threads; i++) {
	    this->chain_domain[i] = new int32 [ this->chains_per_thread ];

	    const int state_size = 256;
	    this->random_state[i] = new char[state_size];
	    initstate((unsigned int) i, (char *) this->random_state[i], (size_t) state_size);
	}
	break;
    }


#if defined(NUMA)
    this->numa_max_domain  = numa_max_node();
    this->num_numa_domains = this->numa_max_domain + 1;
#endif


    switch (this->numa_placement) {
    case LOCAL :
    default:
	this->alloc_local();
	break;
    case XOR :
	this->alloc_xor();
	break;
    case ADD :
	this->alloc_add();
	break;
    case MAP :
	this->alloc_map();
	break;
    }

    return 0;
}


int64
Experiment::parse_number( const char* s )
{
    int64 result = 0;

    int len = strlen( s );
    for (int i=0; i < len; i++) {
	if ( '0' <= s[i] && s[i] <= '9' ) {
	    result = result * 10 + s[i] - '0';
	} else if (s[i] == 'k' || s[i] == 'K') {
	    result = result << 10;
	    break;
	} else if (s[i] == 'm' || s[i] == 'M') {
	    result = result << 20;
	    break;
	} else if (s[i] == 'g' || s[i] == 'G') {
	    result = result << 30;
	    break;
	} else if (s[i] == 't' || s[i] == 'T') {
	    result = result << 40;
	    break;
	} else {
	    break;
	}
    }

    return result;
}


float
Experiment::parse_real( const char* s )
{
    float result = 0;
    bool decimal = false;
    float power = 1;

    int len = strlen( s );
    for (int i=0; i < len; i++) {
	if ( '0' <= s[i] && s[i] <= '9' ) {
	    if (! decimal) {
		result = result * 10 + s[i] - '0';
	    } else {
		power = power / 10;
		result = result + (s[i] - '0') * power;
	    }
	} else if ( '.' == s[i] ) {
	    decimal = true;
	} else {
	    break;
	}
    }

    return result;
}

void
Experiment::alloc_local()
{
    for (int i=0; i < this->num_threads; i++) {
	this->thread_domain[i] = i % this->num_numa_domains;
	for (int j=0; j < this->chains_per_thread; j++) {
	    this->chain_domain[i][j] = this->thread_domain[i];
	}
    }
}

void
Experiment::alloc_xor()
{
    for (int i=0; i < this->num_threads; i++) {
	this->thread_domain[i] = i % this->num_numa_domains;
	for (int j=0; j < this->chains_per_thread; j++) {
	    this->chain_domain[i][j] = (this->thread_domain[i] ^ this->offset_or_mask) % this->num_numa_domains;
	}
    }
}

void
Experiment::alloc_add()
{
    for (int i=0; i < this->num_threads; i++) {
	this->thread_domain[i] = i % this->num_numa_domains;
	for (int j=0; j < this->chains_per_thread; j++) {
	    this->chain_domain[i][j] = (this->thread_domain[i] + this->offset_or_mask) % this->num_numa_domains;
	}
    }
}

				// DOES NOT HANDLE ILL-FORMED SPECIFICATIONS
void
Experiment::alloc_map()
{
				// STRICT -- fail if specifications are inconsistent

				// maps look like "t1:c11,c12,...,c1m;t2:c21,...,c2m;...;tn:cn1,...,cnm"
				// where t[i] is the thread domain of the ith thread,
				// and c[i][j] is the chain domain of the jth chain in the ith thread

				// count the thread descriptors by counting ";" up to EOS
    int threads = 1;
    char *p = this->placement_map;
    while (*p != '\0') {
	if (*p == ';') threads += 1;
	p++;
    }
    int thread_domain[ threads ];

				// count the chain descriptors by counting "," up to ";" or EOS
    int chains = 1;
    p = this->placement_map;
    while (*p != '\0') {
	if (*p == ';') break;
	if (*p == ',') chains += 1;
	p++;
    }
    int chain_domain [ threads ][ chains ];

    int t=0, c=0;
    p = this->placement_map;
    while (*p != '\0') {
    				// everything up to ":" is the thread domain
	int i = 0;
	char buf[64];
	while (*p != '\0') {
	    if (*p == ':') { p++; break; }
	    buf[i] = *p;
	    i++;
	    p++;
	}
	buf[i] = '\0';
	thread_domain[t] = Experiment::parse_number(buf);

    				// search for one or several ','
	c = 0;
	while (*p != '\0' && *p != ';') {
	    if (chains <= c || threads <= t) {
				// error in the thread/chain specification
		fprintf(stderr, "Malformed map.\n");
		exit(1);
	    }
	    int i = 0;
	    while (*p != '\0' && *p != ';') {
		if (*p == ',') { p++; break; }
		buf[i] = *p;
		i++;
		p++;
	    }
	    buf[i] = '\0';
	    chain_domain[t][c] = Experiment::parse_number(buf);
	    c++;
	}

	if (*p == '\0') break;
	if (*p == ';') p++;
	t++;
    }


    this->num_threads = threads;
    this->chains_per_thread = chains;

    this->thread_domain = new int32 [ this->num_threads ];
    this->chain_domain  = new int32*[ this->num_threads ];
    this->random_state  = new char* [ this->num_threads ];

    for (int i=0; i < this->num_threads; i++) {
	this->thread_domain[i] = thread_domain[i] % this->num_numa_domains;

	const int state_size = 256;
	this->random_state[i] = new char[state_size];
	initstate((unsigned int) i, (char *) this->random_state[i], (size_t) state_size);

	this->chain_domain[i] = new int32 [ this->chains_per_thread ];
	for (int j=0; j < this->chains_per_thread; j++) {
	    this->chain_domain[i][j] = chain_domain[i][j] % this->num_numa_domains;
	}
    }

    this->bytes_per_thread = this->bytes_per_chain * this->chains_per_thread;
    this->bytes_per_test   = this->bytes_per_thread * this->num_threads;
}

#include "Chain.h"

void
Experiment::print()
{
    printf("strict            = %d\n", strict);
    printf("pointer_size      = %d\n", pointer_size);
    printf("sizeof(Chain)     = %d\n", sizeof(Chain));
    printf("sizeof(Chain *)   = %d\n", sizeof(Chain *));
    printf("bytes_per_line    = %d\n", bytes_per_line);
    printf("links_per_line    = %d\n", links_per_line);
    printf("bytes_per_page    = %d\n", bytes_per_page);
    printf("lines_per_page    = %d\n", lines_per_page);
    printf("links_per_page    = %d\n", links_per_page);
    printf("bytes_per_chain   = %d\n", bytes_per_chain);
    printf("lines_per_chain   = %d\n", lines_per_chain);
    printf("links_per_chain   = %d\n", links_per_chain);
    printf("pages_per_chain   = %d\n", pages_per_chain);
    printf("chains_per_thread = %d\n", chains_per_thread);
    printf("bytes_per_thread  = %d\n", bytes_per_thread);
    printf("num_threads       = %d\n", num_threads);
    printf("bytes_per_test    = %d\n", bytes_per_test);
    printf("iterations        = %d\n", iterations);
    printf("experiments       = %d\n", experiments);
    printf("access_pattern    = %d\n", access_pattern);
    printf("stride            = %d\n", stride);
    printf("output_mode       = %d\n", output_mode);
    printf("numa_placement    = %d\n", numa_placement);
    printf("offset_or_mask    = %d\n", offset_or_mask);
    printf("numa_max_domain   = %d\n", numa_max_domain);
    printf("num_numa_domains  = %d\n", num_numa_domains);

    for (int i=0; i < this->num_threads; i++) {
	printf("%d: ", this->thread_domain[i]);
	for (int j=0; j < this->chains_per_thread; j++) {
	    printf("%d,", this->chain_domain[i][j]);
	}
	printf("\n");
    }

    fflush(stdout);
}

const char*
Experiment::access()
{
    const char* result = NULL;

    if (this->access_pattern == RANDOM) {
	result = "random";
    } else if (this->access_pattern == STRIDED && 0 < this->stride) {
	result = "forward";
    } else if (this->access_pattern == STRIDED && this->stride < 0) {
	result = "reverse";
    } else if (this->access_pattern == STREAM) {
	result = "stream";
    }

    return result;
}

const char*
Experiment::placement()
{
    const char* result = NULL;

    if (this->numa_placement == LOCAL) {
	result = "local";
    } else if (this->numa_placement == XOR) {
	result = "xor";
    } else if (this->numa_placement == ADD) {
	result = "add";
    } else if (this->numa_placement == MAP) {
	result = "map";
    }

    return result;
}
