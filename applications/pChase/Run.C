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
#include <unistd.h>

#if defined(NUMA)
#include <numa.h>
#endif

#include "Run.h"

#include "Chain.h"
#include "Timer.h"
#include "SpinBarrier.h"


static double max( double v1, double v2 );
static double min( double v1, double v2 );
static void chase_pointers(int64 chains_per_thread, int64 iterations, Chain** root, int64 bytes_per_line, int64 bytes_per_chain, int64 stride);
static void follow_streams(int64 chains_per_thread, int64 iterations, Chain** root, int64 bytes_per_line, int64 bytes_per_chain, int64 stride);
static void (*run_benchmark)(int64 chains_per_thread, int64 iterations, Chain** root, int64 bytes_per_line, int64 bytes_per_chain, int64 stride) = chase_pointers;

Lock   Run::global_mutex;
int64  Run::_ops_per_chain = 0;
double Run::_seconds       = 1E9;

Run::Run()
: exp(NULL), bp(NULL)
{
}

Run::~Run()
{
}

void
Run::set( Experiment &e, SpinBarrier* sbp )
{
    this->exp = &e;
    this->bp  = sbp;
}

int
Run::run()
{ 
				// first allocate all memory for the chains,
				// making sure it is allocated within the 
				// intended numa domains
    Chain** chain_memory = new Chain* [ this->exp->chains_per_thread ];
    Chain** root         = new Chain* [ this->exp->chains_per_thread ];

#if defined(NUMA)
				// establish the node id where this thread
				// will run. threads are mapped to nodes
				// by the set-up code for Experiment.
    int run_node_id = this->exp->thread_domain[this->thread_id()];
    numa_run_on_node(run_node_id);

				// establish the node id where this thread's
				// memory will be allocated.
    for (int i=0; i < this->exp->chains_per_thread; i++) {
	int alloc_node_id = this->exp->chain_domain[this->thread_id()][i];
	nodemask_t alloc_mask;
	nodemask_zero(&alloc_mask);
	nodemask_set(&alloc_mask, alloc_node_id);
	numa_set_membind(&alloc_mask);

	chain_memory[i] = new Chain[ this->exp->links_per_chain ];
    }
#else
    for (int i=0; i < this->exp->chains_per_thread; i++) {
	chain_memory[i] = new Chain[ this->exp->links_per_chain ];
    }
#endif

				// initialize the chains and 
				// select the function that
				// will execute the tests
    for (int i=0; i < this->exp->chains_per_thread; i++) {
	if (this->exp->access_pattern == Experiment::RANDOM) {
	    root[i] = random_mem_init( chain_memory[i] );
	    run_benchmark = chase_pointers;
	} else if (this->exp->access_pattern == Experiment::STRIDED) {
	    if (0 < this->exp->stride) {
		root[i] = forward_mem_init( chain_memory[i] );
	    } else {
		root[i] = reverse_mem_init( chain_memory[i] );
	    }
	    run_benchmark = chase_pointers;
	} else if (this->exp->access_pattern == Experiment::STREAM) {
	    root[i] = stream_mem_init( chain_memory[i] );
	    run_benchmark = follow_streams;
	}
    }

    if (this->exp->iterations <= 0) {
	volatile static double istart  = 0;
	volatile static double istop   = 0;
	volatile static double elapsed = 0;
	volatile static int64  iters   = 1;
	volatile double bound   = max(0.2, 10 * Timer::resolution());
	for (iters=1; elapsed <= bound; iters=iters<<1) {
	    this->bp->barrier();

				// start timer
	    if (this->thread_id() == 0) {
		istart = Timer::seconds();
	    }
	    this->bp->barrier();

				// chase pointers
	    run_benchmark(this->exp->chains_per_thread, iters, root, this->exp->bytes_per_line, this->exp->bytes_per_chain, this->exp->stride);

				// barrier
	    this->bp->barrier();

				// stop timer
	    if (this->thread_id() == 0) {
		istop = Timer::seconds();
		elapsed = istop - istart;
	    }
	    this->bp->barrier();
	}

				// calculate the number of iterations
	if (this->thread_id() == 0) {
	    if (0 < this->exp->seconds) {
		this->exp->iterations = max(1, 0.9999 + 0.5 * this->exp->seconds * iters / elapsed);
	    } else {
		this->exp->iterations = max(1, 0.9999 + iters / elapsed);
	    }
	}
	this->bp->barrier();
    }
#if defined(UNDEFINED)
#endif

				// barrier
    for (int e=0; e < this->exp->experiments; e++) {
	this->bp->barrier();

				// start timer
	double start = 0;
	if (this->thread_id() == 0) start = Timer::seconds();
	this->bp->barrier();

				// chase pointers
	run_benchmark(this->exp->chains_per_thread, this->exp->iterations, root, this->exp->bytes_per_line, this->exp->bytes_per_chain, this->exp->stride);

				// barrier
	this->bp->barrier();

				// stop timer
	double stop = 0;
	if (this->thread_id() == 0) stop = Timer::seconds();
	this->bp->barrier();

	if (0 <= e) {
	    if (this->thread_id() == 0) {
		double delta = stop - start;
		if (0 < delta) {
		    Run::_seconds = min( Run::_seconds, delta );
		}
	    }
	}
    }

    this->bp->barrier();

    for (int i=0; i < this->exp->chains_per_thread; i++) {
	if (chain_memory[i] != NULL) delete [] chain_memory[i];
    }
    if (chain_memory != NULL) delete [] chain_memory;

    return 0;
}

int dummy = 0;
void
Run::mem_check( Chain *m )
{
    if (m == NULL) dummy += 1;
}

static double
max( double v1, double v2 )
{
    if (v1 < v2) return v2;
    return v1;
}

static double
min( double v1, double v2 )
{
    if (v2 < v1) return v2;
    return v1;
}

				// exclude 2 and mersienne primes, i.e.,
				// primes of the form 2**n - 1, e.g.,
				// 3, 7, 31, 127
static const int prime_table[] = { 5, 11, 13, 17, 19, 23, 37, 41, 43, 47,
    53, 61, 71, 73, 79, 83, 89, 97, 101, 103, 109, 113, 131, 137, 139, 149,
    151, 157, 163, };
static const int prime_table_size = sizeof prime_table / sizeof prime_table[0];

Chain*
Run::random_mem_init( Chain *mem )
{
				// initialize pointers --
				// choose a page at random, then use
				// one pointer from each cache line
				// within the page.  all pages and
				// cache lines are chosen at random.
    Chain* root = NULL;
    Chain* prev = NULL;
    int link_within_line = 0;
    int64 local_ops_per_chain = 0;

				// we must set a lock because random()
				// is not thread safe
    Run::global_mutex.lock();
    setstate(this->exp->random_state[this->thread_id()]);
    int page_factor = prime_table[ random() % prime_table_size ];
    int page_offset = random() % this->exp->pages_per_chain;
    Run::global_mutex.unlock();

				// loop through the pages
    for (int i=0; i < this->exp->pages_per_chain; i++) {
	int page = (page_factor * i + page_offset) % this->exp->pages_per_chain;
	Run::global_mutex.lock();
	setstate(this->exp->random_state[this->thread_id()]);
	int line_factor = prime_table[ random() % prime_table_size ];
	int line_offset = random() % this->exp->lines_per_page;
	Run::global_mutex.unlock();

				// loop through the lines within a page
	for (int j=0; j < this->exp->lines_per_page; j++) {
	    int line_within_page = (line_factor * j + line_offset) % this->exp->lines_per_page;
	    int link = page * this->exp->links_per_page + line_within_page * this->exp->links_per_line + link_within_line;

	    if (root == NULL) {
//		printf("root       = %d(%d)[0x%x].\n", page, line_within_page, mem+link);
		prev = root = mem + link;
		local_ops_per_chain += 1;
	    } else {
//		printf("0x%x = %d(%d)[0x%x].\n", prev, page, line_within_page, mem+link);
		prev->next = mem + link;
		prev = prev->next;
		local_ops_per_chain += 1;
	    }
	}
    }

    Run::global_mutex.lock();
    Run::_ops_per_chain = local_ops_per_chain;
    Run::global_mutex.unlock();

    return root;
}

Chain*
Run::forward_mem_init( Chain *mem )
{
    Chain* root = NULL;
    Chain* prev = NULL;
    int link_within_line = 0;
    int64 local_ops_per_chain = 0;

    for (int i=0; i < this->exp->lines_per_chain; i += this->exp->stride) {
	int link = i * this->exp->links_per_line + link_within_line;
	if (root == NULL) {
//	    printf("root       = %d(%d)[0x%x].\n", page, line_within_page, mem+link);
	    prev = root = mem + link;
	    local_ops_per_chain += 1;
	} else {
//	    printf("0x%x = %d(%d)[0x%x].\n", prev, page, line_within_page, mem+link);
	    prev->next = mem + link;
	    prev = prev->next;
	    local_ops_per_chain += 1;
	}
    }

    Run::global_mutex.lock();
    Run::_ops_per_chain = local_ops_per_chain;
    Run::global_mutex.unlock();

    return root;
}

Chain*
Run::reverse_mem_init( Chain *mem )
{
    Chain* root = NULL;
    Chain* prev = NULL;
    int link_within_line = 0;
    int64 local_ops_per_chain = 0;

    int stride = -this->exp->stride;
    int last;
    for (int i=0; i < this->exp->lines_per_chain; i += stride) {
	last = i;
    }

    for (int i=last; 0 <= i; i -= stride) {
	int link = i * this->exp->links_per_line + link_within_line;
	if (root == NULL) {
//	    printf("root       = %d(%d)[0x%x].\n", page, line_within_page, mem+link);
	    prev = root = mem + link;
	    local_ops_per_chain += 1;
	} else {
//	    printf("0x%x = %d(%d)[0x%x].\n", prev, page, line_within_page, mem+link);
	    prev->next = mem + link;
	    prev = prev->next;
	    local_ops_per_chain += 1;
	}
    }

    Run::global_mutex.lock();
    Run::_ops_per_chain = local_ops_per_chain;
    Run::global_mutex.unlock();

    return root;
}

static int64 dumb_ck = 0;
void
mem_chk( Chain *m )
{
    if (m == NULL) dumb_ck += 1;
}

static void
chase_pointers(
    int64 chains_per_thread,		// memory loading per thread
    int64 iterations,			// number of iterations per experiment
    Chain** root,			// root(s) of the chain(s) to follow
    int64 bytes_per_line,		// ignored
    int64 bytes_per_chain,		// ignored
    int64 stride			// ignored
)
{
				// chase pointers
    switch (chains_per_thread) {
    default:
    case 1:
	for (int64 i=0; i < iterations; i++) {
	    Chain* a = root[0];
	    while (a != NULL) {
		a = a->next;
	    }
	    mem_chk( a );
	}
	break;
    case 2:
	for (int64 i=0; i < iterations; i++) {
	    Chain* a = root[0];
	    Chain* b = root[1];
	    while (a != NULL) {
		a = a->next;
		b = b->next;
	    }
	    mem_chk( a );
	    mem_chk( b );
	}
	break;
    case 3:
	for (int64 i=0; i < iterations; i++) {
	    Chain* a = root[0];
	    Chain* b = root[1];
	    Chain* c = root[2];
	    while (a != NULL) {
		a = a->next;
		b = b->next;
		c = c->next;
	    }
	    mem_chk( a );
	    mem_chk( b );
	    mem_chk( c );
	}
	break;
    case 4:
	for (int64 i=0; i < iterations; i++) {
	    Chain* a = root[0];
	    Chain* b = root[1];
	    Chain* c = root[2];
	    Chain* d = root[3];
	    while (a != NULL) {
		a = a->next;
		b = b->next;
		c = c->next;
		d = d->next;
	    }
	    mem_chk( a );
	    mem_chk( b );
	    mem_chk( c );
	    mem_chk( d );
	}
	break;
    case 5:
	for (int64 i=0; i < iterations; i++) {
	    Chain* a = root[0];
	    Chain* b = root[1];
	    Chain* c = root[2];
	    Chain* d = root[3];
	    Chain* e = root[4];
	    while (a != NULL) {
		a = a->next;
		b = b->next;
		c = c->next;
		d = d->next;
		e = e->next;
	    }
	    mem_chk( a );
	    mem_chk( b );
	    mem_chk( c );
	    mem_chk( d );
	    mem_chk( e );
	}
	break;
    case 6:
	for (int64 i=0; i < iterations; i++) {
	    Chain* a = root[0];
	    Chain* b = root[1];
	    Chain* c = root[2];
	    Chain* d = root[3];
	    Chain* e = root[4];
	    Chain* f = root[5];
	    while (a != NULL) {
		a = a->next;
		b = b->next;
		c = c->next;
		d = d->next;
		e = e->next;
		f = f->next;
	    }
	    mem_chk( a );
	    mem_chk( b );
	    mem_chk( c );
	    mem_chk( d );
	    mem_chk( e );
	    mem_chk( f );
	}
	break;
    case 7:
	for (int64 i=0; i < iterations; i++) {
	    Chain* a = root[0];
	    Chain* b = root[1];
	    Chain* c = root[2];
	    Chain* d = root[3];
	    Chain* e = root[4];
	    Chain* f = root[5];
	    Chain* g = root[6];
	    while (a != NULL) {
		a = a->next;
		b = b->next;
		c = c->next;
		d = d->next;
		e = e->next;
		f = f->next;
		g = g->next;
	    }
	    mem_chk( a );
	    mem_chk( b );
	    mem_chk( c );
	    mem_chk( d );
	    mem_chk( e );
	    mem_chk( f );
	    mem_chk( g );
	}
	break;
    case 8:
	for (int64 i=0; i < iterations; i++) {
	    Chain* a = root[0];
	    Chain* b = root[1];
	    Chain* c = root[2];
	    Chain* d = root[3];
	    Chain* e = root[4];
	    Chain* f = root[5];
	    Chain* g = root[6];
	    Chain* h = root[7];
	    while (a != NULL) {
		a = a->next;
		b = b->next;
		c = c->next;
		d = d->next;
		e = e->next;
		f = f->next;
		g = g->next;
		h = h->next;
	    }
	    mem_chk( a );
	    mem_chk( b );
	    mem_chk( c );
	    mem_chk( d );
	    mem_chk( e );
	    mem_chk( f );
	    mem_chk( g );
	    mem_chk( h );
	}
	break;
    case 9:
	for (int64 i=0; i < iterations; i++) {
	    Chain* a = root[0];
	    Chain* b = root[1];
	    Chain* c = root[2];
	    Chain* d = root[3];
	    Chain* e = root[4];
	    Chain* f = root[5];
	    Chain* g = root[6];
	    Chain* h = root[7];
	    Chain* j = root[8];
	    while (a != NULL) {
		a = a->next;
		b = b->next;
		c = c->next;
		d = d->next;
		e = e->next;
		f = f->next;
		g = g->next;
		h = h->next;
		j = j->next;
	    }
	    mem_chk( a );
	    mem_chk( b );
	    mem_chk( c );
	    mem_chk( d );
	    mem_chk( e );
	    mem_chk( f );
	    mem_chk( g );
	    mem_chk( h );
	    mem_chk( j );
	}
	break;
    case 10:
	for (int64 i=0; i < iterations; i++) {
	    Chain* a = root[0];
	    Chain* b = root[1];
	    Chain* c = root[2];
	    Chain* d = root[3];
	    Chain* e = root[4];
	    Chain* f = root[5];
	    Chain* g = root[6];
	    Chain* h = root[7];
	    Chain* j = root[8];
	    Chain* k = root[9];
	    while (a != NULL) {
		a = a->next;
		b = b->next;
		c = c->next;
		d = d->next;
		e = e->next;
		f = f->next;
		g = g->next;
		h = h->next;
		j = j->next;
		k = k->next;
	    }
	    mem_chk( a );
	    mem_chk( b );
	    mem_chk( c );
	    mem_chk( d );
	    mem_chk( e );
	    mem_chk( f );
	    mem_chk( g );
	    mem_chk( h );
	    mem_chk( j );
	    mem_chk( k );
	}
	break;
    case 11:
	for (int64 i=0; i < iterations; i++) {
	    Chain* a = root[0];
	    Chain* b = root[1];
	    Chain* c = root[2];
	    Chain* d = root[3];
	    Chain* e = root[4];
	    Chain* f = root[5];
	    Chain* g = root[6];
	    Chain* h = root[7];
	    Chain* j = root[8];
	    Chain* k = root[9];
	    Chain* l = root[10];
	    while (a != NULL) {
		a = a->next;
		b = b->next;
		c = c->next;
		d = d->next;
		e = e->next;
		f = f->next;
		g = g->next;
		h = h->next;
		j = j->next;
		k = k->next;
		l = l->next;
	    }
	    mem_chk( a );
	    mem_chk( b );
	    mem_chk( c );
	    mem_chk( d );
	    mem_chk( e );
	    mem_chk( f );
	    mem_chk( g );
	    mem_chk( h );
	    mem_chk( j );
	    mem_chk( k );
	    mem_chk( l );
	}
	break;
    case 12:
	for (int64 i=0; i < iterations; i++) {
	    Chain* a = root[0];
	    Chain* b = root[1];
	    Chain* c = root[2];
	    Chain* d = root[3];
	    Chain* e = root[4];
	    Chain* f = root[5];
	    Chain* g = root[6];
	    Chain* h = root[7];
	    Chain* j = root[8];
	    Chain* k = root[9];
	    Chain* l = root[10];
	    Chain* m = root[11];
	    while (a != NULL) {
		a = a->next;
		b = b->next;
		c = c->next;
		d = d->next;
		e = e->next;
		f = f->next;
		g = g->next;
		h = h->next;
		j = j->next;
		k = k->next;
		l = l->next;
		m = m->next;
	    }
	    mem_chk( a );
	    mem_chk( b );
	    mem_chk( c );
	    mem_chk( d );
	    mem_chk( e );
	    mem_chk( f );
	    mem_chk( g );
	    mem_chk( h );
	    mem_chk( j );
	    mem_chk( k );
	    mem_chk( l );
	    mem_chk( m );
	}
	break;
    case 13:
	for (int64 i=0; i < iterations; i++) {
	    Chain* a = root[0];
	    Chain* b = root[1];
	    Chain* c = root[2];
	    Chain* d = root[3];
	    Chain* e = root[4];
	    Chain* f = root[5];
	    Chain* g = root[6];
	    Chain* h = root[7];
	    Chain* j = root[8];
	    Chain* k = root[9];
	    Chain* l = root[10];
	    Chain* m = root[11];
	    Chain* n = root[12];
	    while (a != NULL) {
		a = a->next;
		b = b->next;
		c = c->next;
		d = d->next;
		e = e->next;
		f = f->next;
		g = g->next;
		h = h->next;
		j = j->next;
		k = k->next;
		l = l->next;
		m = m->next;
		n = n->next;
	    }
	    mem_chk( a );
	    mem_chk( b );
	    mem_chk( c );
	    mem_chk( d );
	    mem_chk( e );
	    mem_chk( f );
	    mem_chk( g );
	    mem_chk( h );
	    mem_chk( j );
	    mem_chk( k );
	    mem_chk( l );
	    mem_chk( m );
	    mem_chk( n );
	}
	break;
    case 14:
	for (int64 i=0; i < iterations; i++) {
	    Chain* a = root[0];
	    Chain* b = root[1];
	    Chain* c = root[2];
	    Chain* d = root[3];
	    Chain* e = root[4];
	    Chain* f = root[5];
	    Chain* g = root[6];
	    Chain* h = root[7];
	    Chain* j = root[8];
	    Chain* k = root[9];
	    Chain* l = root[10];
	    Chain* m = root[11];
	    Chain* n = root[12];
	    Chain* o = root[13];
	    while (a != NULL) {
		a = a->next;
		b = b->next;
		c = c->next;
		d = d->next;
		e = e->next;
		f = f->next;
		g = g->next;
		h = h->next;
		j = j->next;
		k = k->next;
		l = l->next;
		m = m->next;
		n = n->next;
		o = o->next;
	    }
	    mem_chk( a );
	    mem_chk( b );
	    mem_chk( c );
	    mem_chk( d );
	    mem_chk( e );
	    mem_chk( f );
	    mem_chk( g );
	    mem_chk( h );
	    mem_chk( j );
	    mem_chk( k );
	    mem_chk( l );
	    mem_chk( m );
	    mem_chk( n );
	    mem_chk( o );
	}
	break;
    case 15:
	for (int64 i=0; i < iterations; i++) {
	    Chain* a = root[0];
	    Chain* b = root[1];
	    Chain* c = root[2];
	    Chain* d = root[3];
	    Chain* e = root[4];
	    Chain* f = root[5];
	    Chain* g = root[6];
	    Chain* h = root[7];
	    Chain* j = root[8];
	    Chain* k = root[9];
	    Chain* l = root[10];
	    Chain* m = root[11];
	    Chain* n = root[12];
	    Chain* o = root[13];
	    Chain* p = root[14];
	    while (a != NULL) {
		a = a->next;
		b = b->next;
		c = c->next;
		d = d->next;
		e = e->next;
		f = f->next;
		g = g->next;
		h = h->next;
		j = j->next;
		k = k->next;
		l = l->next;
		m = m->next;
		n = n->next;
		o = o->next;
		p = p->next;
	    }
	    mem_chk( a );
	    mem_chk( b );
	    mem_chk( c );
	    mem_chk( d );
	    mem_chk( e );
	    mem_chk( f );
	    mem_chk( g );
	    mem_chk( h );
	    mem_chk( j );
	    mem_chk( k );
	    mem_chk( l );
	    mem_chk( m );
	    mem_chk( n );
	    mem_chk( o );
	    mem_chk( p );
	}
	break;
    case 16:
	for (int64 i=0; i < iterations; i++) {
	    Chain* a = root[0];
	    Chain* b = root[1];
	    Chain* c = root[2];
	    Chain* d = root[3];
	    Chain* e = root[4];
	    Chain* f = root[5];
	    Chain* g = root[6];
	    Chain* h = root[7];
	    Chain* j = root[8];
	    Chain* k = root[9];
	    Chain* l = root[10];
	    Chain* m = root[11];
	    Chain* n = root[12];
	    Chain* o = root[13];
	    Chain* p = root[14];
	    Chain* q = root[15];
	    while (a != NULL) {
		a = a->next;
		b = b->next;
		c = c->next;
		d = d->next;
		e = e->next;
		f = f->next;
		g = g->next;
		h = h->next;
		j = j->next;
		k = k->next;
		l = l->next;
		m = m->next;
		n = n->next;
		o = o->next;
		p = p->next;
		q = q->next;
	    }
	    mem_chk( a );
	    mem_chk( b );
	    mem_chk( c );
	    mem_chk( d );
	    mem_chk( e );
	    mem_chk( f );
	    mem_chk( g );
	    mem_chk( h );
	    mem_chk( j );
	    mem_chk( k );
	    mem_chk( l );
	    mem_chk( m );
	    mem_chk( n );
	    mem_chk( o );
	    mem_chk( p );
	    mem_chk( q );
	}
    }
}

				// NOT WRITTEN YET -- DMP
				// JUST A PLACE HOLDER!
Chain*
Run::stream_mem_init( Chain *mem )
{
// fprintf(stderr, "made it into stream_mem_init.\n");
// fprintf(stderr, "chains_per_thread = %ld\n", this->exp->chains_per_thread);
// fprintf(stderr, "iterations        = %ld\n", this->exp->iterations);
// fprintf(stderr, "bytes_per_chain   = %ld\n", this->exp->bytes_per_chain);
// fprintf(stderr, "stride            = %ld\n", this->exp->stride);
    int64 local_ops_per_chain = 0;
    double* tmp = (double *) mem;
    int64 refs_per_line  = this->exp->bytes_per_line  / sizeof(double);
    int64 refs_per_chain = this->exp->bytes_per_chain / sizeof(double);
// fprintf(stderr, "refs_per_chain    = %ld\n", refs_per_chain);

    for (int64 i=0; i < refs_per_chain; i += this->exp->stride*refs_per_line) {
	tmp[i] = 0;
	local_ops_per_chain += 1;
    }

    Run::global_mutex.lock();
    Run::_ops_per_chain = local_ops_per_chain;
    Run::global_mutex.unlock();

// fprintf(stderr, "made it out of stream_mem_init.\n");
    return mem;
}

static int64 summ_ck = 0;
void
sum_chk( double t )
{
    if (t != 0) summ_ck += 1;
}

				// NOT WRITTEN YET -- DMP
				// JUST A PLACE HOLDER!
static void
follow_streams(
    int64 chains_per_thread,		// memory loading per thread
    int64 iterations,			// number of iterations per experiment
    Chain** root,			// root(s) of the chain(s) to follow
    int64 bytes_per_line,		// ignored
    int64 bytes_per_chain,		// ignored
    int64 stride			// ignored
)
{
    int64 refs_per_line  = bytes_per_line  / sizeof(double);
    int64 refs_per_chain = bytes_per_chain / sizeof(double);

				// chase pointers
    switch (chains_per_thread) {
    default:
    case 1:
	for (int64 i=0; i < iterations; i++) {
	    double t = 0;
	    double* a0 = (double *) root[0];
	    for (int64 j=0; j < refs_per_chain; j+=stride*refs_per_line) {
		t += a0[j];
	    }
	    sum_chk( t );
	}
	break;
    case 2:
	for (int64 i=0; i < iterations; i++) {
	    double t = 0;
	    double* a0 = (double *) root[0];
	    double* a1 = (double *) root[1];
	    for (int64 j=0; j < refs_per_chain; j+=stride*refs_per_line) {
		t += a0[j] + a1[j];
	    }
	    sum_chk( t );
	}
	break;
    case 3:
	for (int64 i=0; i < iterations; i++) {
	    double t = 0;
	    double* a0 = (double *) root[0];
	    double* a1 = (double *) root[1];
	    double* a2 = (double *) root[2];
	    for (int64 j=0; j < refs_per_chain; j+=stride*refs_per_line) {
		t += a0[j] + a1[j] + a2[j];
	    }
	    sum_chk( t );
	}
	break;
    case 4:
	for (int64 i=0; i < iterations; i++) {
	    double t = 0;
	    double* a0 = (double *) root[0];
	    double* a1 = (double *) root[1];
	    double* a2 = (double *) root[2];
	    double* a3 = (double *) root[3];
	    for (int64 j=0; j < refs_per_chain; j+=stride*refs_per_line) {
		t += a0[j] + a1[j] + a2[j] + a3[j];
	    }
	    sum_chk( t );
	}
	break;
    case 5:
	for (int64 i=0; i < iterations; i++) {
	    double t = 0;
	    double* a0 = (double *) root[0];
	    double* a1 = (double *) root[1];
	    double* a2 = (double *) root[2];
	    double* a3 = (double *) root[3];
	    double* a4 = (double *) root[4];
	    for (int64 j=0; j < refs_per_chain; j+=stride*refs_per_line) {
		t += a0[j] + a1[j] + a2[j] + a3[j] + a4[j];
	    }
	    sum_chk( t );
	}
	break;
    case 6:
	for (int64 i=0; i < iterations; i++) {
	    double t = 0;
	    double* a0 = (double *) root[0];
	    double* a1 = (double *) root[1];
	    double* a2 = (double *) root[2];
	    double* a3 = (double *) root[3];
	    double* a4 = (double *) root[4];
	    double* a5 = (double *) root[5];
	    for (int64 j=0; j < refs_per_chain; j+=stride*refs_per_line) {
		t += a0[j] + a1[j] + a2[j] + a3[j] + a4[j] + a5[j];
	    }
	    sum_chk( t );
	}
	break;
    case 7:
	for (int64 i=0; i < iterations; i++) {
	    double t = 0;
	    double* a0 = (double *) root[0];
	    double* a1 = (double *) root[1];
	    double* a2 = (double *) root[2];
	    double* a3 = (double *) root[3];
	    double* a4 = (double *) root[4];
	    double* a5 = (double *) root[5];
	    double* a6 = (double *) root[6];
	    for (int64 j=0; j < refs_per_chain; j+=stride*refs_per_line) {
		t += a0[j] + a1[j] + a2[j] + a3[j] + a4[j] + a5[j] + a6[j];
	    }
	    sum_chk( t );
	}
	break;
    case 8:
	for (int64 i=0; i < iterations; i++) {
	    double t = 0;
	    double* a0 = (double *) root[0];
	    double* a1 = (double *) root[1];
	    double* a2 = (double *) root[2];
	    double* a3 = (double *) root[3];
	    double* a4 = (double *) root[4];
	    double* a5 = (double *) root[5];
	    double* a6 = (double *) root[6];
	    double* a7 = (double *) root[7];
	    for (int64 j=0; j < refs_per_chain; j+=stride*refs_per_line) {
		t += a0[j] + a1[j] + a2[j] + a3[j] + a4[j] + a5[j] + a6[j] + a7[j];
	    }
	    sum_chk( t );
	}
	break;
    case 9:
	for (int64 i=0; i < iterations; i++) {
	    double t = 0;
	    double* a0 = (double *) root[0];
	    double* a1 = (double *) root[1];
	    double* a2 = (double *) root[2];
	    double* a3 = (double *) root[3];
	    double* a4 = (double *) root[4];
	    double* a5 = (double *) root[5];
	    double* a6 = (double *) root[6];
	    double* a7 = (double *) root[7];
	    double* a8 = (double *) root[8];
	    for (int64 j=0; j < refs_per_chain; j+=stride*refs_per_line) {
		t += a0[j] + a1[j] + a2[j] + a3[j] + a4[j] + a5[j] + a6[j] + a7[j] +
		     a8[j];
	    }
	    sum_chk( t );
	}
	break;
    case 10:
	for (int64 i=0; i < iterations; i++) {
	    double t = 0;
	    double* a0 = (double *) root[0];
	    double* a1 = (double *) root[1];
	    double* a2 = (double *) root[2];
	    double* a3 = (double *) root[3];
	    double* a4 = (double *) root[4];
	    double* a5 = (double *) root[5];
	    double* a6 = (double *) root[6];
	    double* a7 = (double *) root[7];
	    double* a8 = (double *) root[8];
	    double* a9 = (double *) root[9];
	    for (int64 j=0; j < refs_per_chain; j+=stride*refs_per_line) {
		t += a0[j] + a1[j] + a2[j] + a3[j] + a4[j] + a5[j] + a6[j] + a7[j] +
		     a8[j] + a9[j];
	    }
	    sum_chk( t );
	}
	break;
    case 11:
	for (int64 i=0; i < iterations; i++) {
	    double t = 0;
	    double* a0  = (double *) root[ 0];
	    double* a1  = (double *) root[ 1];
	    double* a2  = (double *) root[ 2];
	    double* a3  = (double *) root[ 3];
	    double* a4  = (double *) root[ 4];
	    double* a5  = (double *) root[ 5];
	    double* a6  = (double *) root[ 6];
	    double* a7  = (double *) root[ 7];
	    double* a8  = (double *) root[ 8];
	    double* a9  = (double *) root[ 9];
	    double* a10 = (double *) root[10];
	    for (int64 j=0; j < refs_per_chain; j+=stride*refs_per_line) {
		t += a0[j] + a1[j] + a2 [j] + a3[j] + a4[j] + a5[j] + a6[j] + a7[j] +
		     a8[j] + a9[j] + a10[j];
	    }
	    sum_chk( t );
	}
	break;
    case 12:
	for (int64 i=0; i < iterations; i++) {
	    double t = 0;
	    double* a0  = (double *) root[ 0];
	    double* a1  = (double *) root[ 1];
	    double* a2  = (double *) root[ 2];
	    double* a3  = (double *) root[ 3];
	    double* a4  = (double *) root[ 4];
	    double* a5  = (double *) root[ 5];
	    double* a6  = (double *) root[ 6];
	    double* a7  = (double *) root[ 7];
	    double* a8  = (double *) root[ 8];
	    double* a9  = (double *) root[ 9];
	    double* a10 = (double *) root[10];
	    double* a11 = (double *) root[11];
	    for (int64 j=0; j < refs_per_chain; j+=stride*refs_per_line) {
		t += a0[j] + a1[j] + a2 [j] + a3 [j] + a4[j] + a5[j] + a6[j] + a7[j] +
		     a8[j] + a9[j] + a10[j] + a11[j];
	    }
	    sum_chk( t );
	}
	break;
    case 13:
	for (int64 i=0; i < iterations; i++) {
	    double t = 0;
	    double* a0  = (double *) root[ 0];
	    double* a1  = (double *) root[ 1];
	    double* a2  = (double *) root[ 2];
	    double* a3  = (double *) root[ 3];
	    double* a4  = (double *) root[ 4];
	    double* a5  = (double *) root[ 5];
	    double* a6  = (double *) root[ 6];
	    double* a7  = (double *) root[ 7];
	    double* a8  = (double *) root[ 8];
	    double* a9  = (double *) root[ 9];
	    double* a10 = (double *) root[10];
	    double* a11 = (double *) root[11];
	    double* a12 = (double *) root[12];
	    for (int64 j=0; j < refs_per_chain; j+=stride*refs_per_line) {
		t += a0[j] + a1[j] + a2 [j] + a3 [j] + a4 [j] + a5[j] + a6[j] + a7[j] +
		     a8[j] + a9[j] + a10[j] + a11[j] + a12[j];
	    }
	    sum_chk( t );
	}
	break;
    case 14:
	for (int64 i=0; i < iterations; i++) {
	    double t = 0;
	    double* a0  = (double *) root[ 0];
	    double* a1  = (double *) root[ 1];
	    double* a2  = (double *) root[ 2];
	    double* a3  = (double *) root[ 3];
	    double* a4  = (double *) root[ 4];
	    double* a5  = (double *) root[ 5];
	    double* a6  = (double *) root[ 6];
	    double* a7  = (double *) root[ 7];
	    double* a8  = (double *) root[ 8];
	    double* a9  = (double *) root[ 9];
	    double* a10 = (double *) root[10];
	    double* a11 = (double *) root[11];
	    double* a12 = (double *) root[12];
	    double* a13 = (double *) root[13];
	    for (int64 j=0; j < refs_per_chain; j+=stride*refs_per_line) {
		t += a0[j] + a1[j] + a2 [j] + a3 [j] + a4 [j] + a5 [j] + a6[j] + a7[j] +
		     a8[j] + a9[j] + a10[j] + a11[j] + a12[j] + a13[j];
	    }
	    sum_chk( t );
	}
	break;
    case 15:
	for (int64 i=0; i < iterations; i++) {
	    double t = 0;
	    double* a0  = (double *) root[ 0];
	    double* a1  = (double *) root[ 1];
	    double* a2  = (double *) root[ 2];
	    double* a3  = (double *) root[ 3];
	    double* a4  = (double *) root[ 4];
	    double* a5  = (double *) root[ 5];
	    double* a6  = (double *) root[ 6];
	    double* a7  = (double *) root[ 7];
	    double* a8  = (double *) root[ 8];
	    double* a9  = (double *) root[ 9];
	    double* a10 = (double *) root[10];
	    double* a11 = (double *) root[11];
	    double* a12 = (double *) root[12];
	    double* a13 = (double *) root[13];
	    double* a14 = (double *) root[14];
	    for (int64 j=0; j < refs_per_chain; j+=stride*refs_per_line) {
		t += a0[j] + a1[j] + a2 [j] + a3 [j] + a4 [j] + a5 [j] + a6 [j] + a7[j] +
		     a8[j] + a9[j] + a10[j] + a11[j] + a12[j] + a13[j] + a14[j];
	    }
	    sum_chk( t );
	}
	break;
    case 16:
	for (int64 i=0; i < iterations; i++) {
	    double t = 0;
	    double* a0  = (double *) root[ 0];
	    double* a1  = (double *) root[ 1];
	    double* a2  = (double *) root[ 2];
	    double* a3  = (double *) root[ 3];
	    double* a4  = (double *) root[ 4];
	    double* a5  = (double *) root[ 5];
	    double* a6  = (double *) root[ 6];
	    double* a7  = (double *) root[ 7];
	    double* a8  = (double *) root[ 8];
	    double* a9  = (double *) root[ 9];
	    double* a10 = (double *) root[10];
	    double* a11 = (double *) root[11];
	    double* a12 = (double *) root[12];
	    double* a13 = (double *) root[13];
	    double* a14 = (double *) root[14];
	    double* a15 = (double *) root[15];
	    for (int64 j=0; j < refs_per_chain; j+=stride*refs_per_line) {
		t += a0[j] + a1[j] + a2 [j] + a3 [j] + a4 [j] + a5 [j] + a6 [j] + a7 [j] +
		     a8[j] + a9[j] + a10[j] + a11[j] + a12[j] + a13[j] + a14[j] + a15[j];
	    }
	    sum_chk( t );
	}
	break;
    }
}
