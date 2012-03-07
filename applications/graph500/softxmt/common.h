/* Copyright (C) 2010-2011 The Trustees of Indiana University.             */
/*                                                                         */
/* Use, modification and distribution is subject to the Boost Software     */
/* License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at */
/* http://www.boost.org/LICENSE_1_0.txt)                                   */
/*                                                                         */
/*  Authors: Jeremiah Willcock                                             */
/*           Andrew Lumsdaine                                              */

#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stddef.h>
#include "../generator/graph_generator.h"
#include "../compat.h"

#define SIZE_MUST_BE_A_POWER_OF_TWO

#ifdef SIZE_MUST_BE_A_POWER_OF_TWO
extern int lgsize;
#endif

/* Distribute edges by their endpoints (make two directed copies of each input
 * undirected edge); distribution is 1-d and cyclic. */
#ifdef SIZE_MUST_BE_A_POWER_OF_TWO
#define MOD_SIZE(v) ((v) & ((1 << lgsize) - 1))
#define DIV_SIZE(v) ((v) >> lgsize)
#define MUL_SIZE(x) ((x) << lgsize)
#else
#define MOD_SIZE(v) ((v) % size)
#define DIV_SIZE(v) ((v) / size)
#define MUL_SIZE(x) ((x) * size)
#endif
#define VERTEX_OWNER(v) ((int)(MOD_SIZE(v)))
#define VERTEX_LOCAL(v) ((size_t)(DIV_SIZE(v)))
#define VERTEX_TO_GLOBAL(r, i) ((int64_t)(MUL_SIZE((uint64_t)i) + (int)(r)))

typedef struct tuple_graph {
  packed_edge * restrict edges;
  int64_t nedge; /* Number of edges in graph, in both cases */
} tuple_graph;

#ifdef __cplusplus
extern "C" {
#endif

int lg_int64_t(int64_t x); /* In utils.c */
void* xmalloc(size_t nbytes); /* In utils.c */
void* xcalloc(size_t n, size_t unit); /* In utils.c */
void* xrealloc(void* p, size_t nbytes); /* In utils.c */

//int validate_bfs_result(const tuple_graph* const tg, const int64_t nglobalverts, const size_t nlocalverts, const int64_t root, int64_t* const pred, int64_t* const edge_visit_count_ptr); /* In validate.c */

/* Definitions in each BFS file, using static global variables for internal
 * storage: */
void make_graph_data_structure(const tuple_graph* const tg);
void free_graph_data_structure(void);
int bfs_writes_depth_map(void); /* True if high 16 bits of pred entries are zero-based level numbers, or UINT16_MAX for unreachable. */
void run_bfs(int64_t root, int64_t* pred);
void get_vertex_distribution_for_pred(size_t count, const int64_t* vertices, int* owners, size_t* locals);
int64_t vertex_to_global_for_pred(int v_rank, size_t v_local); /* Only used for error messages */
size_t get_nlocalverts_for_pred(void);

static inline size_t size_min(size_t a, size_t b) {
  return a < b ? a : b;
}
static inline ptrdiff_t ptrdiff_min(ptrdiff_t a, ptrdiff_t b) {
  return a < b ? a : b;
}
static inline int64_t int64_min(int64_t a, int64_t b) {
  return a < b ? a : b;
}

#ifdef __cplusplus
}
#endif

#endif /* COMMON_H */
