/* Copyright (C) 2010-2011 The Trustees of Indiana University.             */
/*                                                                         */
/* Use, modification and distribution is subject to the Boost Software     */
/* License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at */
/* http://www.boost.org/LICENSE_1_0.txt)                                   */
/*                                                                         */
/*  Authors: Jeremiah Willcock                                             */
/*           Andrew Lumsdaine                                              */

//#include "common.h"
//#include "oned_csr.h"
//#include <stdint.h>
//#include <inttypes.h>
//#include <stdlib.h>
//#include <stddef.h>
//#include <string.h>
//#include <limits.h>
//#include <assert.h>
//
//static csr_graph g;
//static int64_t* g_oldq;
//static int64_t* g_newq;
//static unsigned long* g_visited;
//static const int coalescing_size = 256;
//
//void make_graph_data_structure(const tuple_graph* const tg) {
//  convert_graph_to_oned_csr(tg, &g);
//  const size_t nlocalverts = g.nlocalverts;
//  g_oldq = (int64_t*)xmalloc(nlocalverts * sizeof(int64_t));
//  g_newq = (int64_t*)xmalloc(nlocalverts * sizeof(int64_t));
//  const int ulong_bits = sizeof(unsigned long) * CHAR_BIT;
//  int64_t visited_size = (nlocalverts + ulong_bits - 1) / ulong_bits;
//  g_visited = (unsigned long*)xmalloc(visited_size * sizeof(unsigned long));
//}
//
//void free_graph_data_structure(void) {
//  free(g_oldq);
//  free(g_newq);
//  free(g_visited);
//  free_oned_csr_graph(&g);
//}
//
//int bfs_writes_depth_map(void) {
//  return 0;
//}
//
///* This version is the traditional level-synchronized BFS using two queues.  A
// * bitmap is used to indicate which vertices have been visited.  Messages are
// * sent and processed asynchronously throughout the code to hopefully overlap
// * communication with computation. */
//void run_bfs(int64_t root, int64_t* pred) {
//  
//}
//
//void get_vertex_distribution_for_pred(size_t count, const int64_t* vertex_p, int* owner_p, size_t* local_p) {
//  const int64_t* restrict vertex = vertex_p;
//  int* restrict owner = owner_p;
//  size_t* restrict local = local_p;
//  ptrdiff_t i;
//#pragma omp parallel for
//  for (i = 0; i < (ptrdiff_t)count; ++i) {
//    owner[i] = VERTEX_OWNER(vertex[i]);
//    local[i] = VERTEX_LOCAL(vertex[i]);
//  }
//}
//
//int64_t vertex_to_global_for_pred(int v_rank, size_t v_local) {
//  return VERTEX_TO_GLOBAL(v_rank, v_local);
//}
//
//size_t get_nlocalverts_for_pred(void) {
//  return g.nlocalverts;
//}
