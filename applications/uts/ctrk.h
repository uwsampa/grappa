/*
 *         ---- The Unbalanced Tree Search (UTS) Benchmark ----
 *  
 *  Copyright (c) 2010 See AUTHORS file for copyright holders
 *
 *  This file is part of the unbalanced tree search benchmark.  This
 *  project is licensed under the MIT Open Source license.  See the LICENSE
 *  file for copyright and licensing information.
 *
 *  UTS is a collaborative project between researchers at the University of
 *  Maryland, the University of North Carolina at Chapel Hill, and the Ohio
 *  State University.  See AUTHORS file for more information.
 *
 */

#ifndef _CTRK_H
#define _CTRK_H

/** CTRK -- Debugging Chunk Tracker: This code is used to dump an ID for each
 *          chunk so we can track them through dynamic load balancing.  The
 *          output should be fed into check_ctrk.pl which will tally the chunks
 *          and tell us if any went missing.
 *
 *          These functions use the state of the node on the top of the chunk
 *          to track each chunk.
 **/

#ifndef _UTS_H
// Make sure we don't stomp on uts.h
#include "rng/rng.h"

struct node_t{
  int type;          // distribution governing number of children
  int height;        // depth of this node in the tree
  int numChildren;   // number of children, -1 => not yet determined
  
  /* for RNG state associated with this node */
  struct state_t state;
};
typedef struct node_t Node;
#endif /* _UTS_H */

static char ctrk_buff[500];

#ifdef CTRK
/* void ctrk_get(int rank, Node* node_c); */  
#define ctrk_get(rank, node_c)                                                       \
	dump_hex(ctrk_buff, ((Node*)node_c)->state.state, sizeof(struct state_t));   \
	printf("CTRK: PE %d got chunk 0x%s\n", rank, ctrk_buff); fflush(NULL);
#else
#define ctrk_get(rank, node_c)
#endif

#ifdef CTRK
#define ctrk_put(rank, node_c)                                                       \
	dump_hex(ctrk_buff, ((Node*)node_c)->state.state, sizeof(struct state_t));   \
	printf("CTRK: PE %d put chunk 0x%s\n", rank, ctrk_buff); fflush(NULL);
#else
#define ctrk_put(rank, node_c)
#endif


/* Dump some memory location to a hexadecimal string */
static void dump_hex(char *out, void *in, int nbytes)
{
	/* out should be twice as long as nbytes since we generate 2 chars per byte */
	int i;
	char l,r;
	char *cin = in;

	for (i = 0; i < nbytes; i++) {
		l = (cin[i] & 0xF0) >> 4;
		r = (cin[i] & 0x0F);
		out[2*i] = (l < 10) ? l + '0' : l - 10 + 'A';
		out[2*i+1] = (r < 10) ? r + '0' : r - 10 + 'A';
	}

	out[2*i] = '\0';
}

#endif /* _CTRK_H */
