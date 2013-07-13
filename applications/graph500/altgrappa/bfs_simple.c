/* Copyright (C) 2010-2011 The Trustees of Indiana University.             */
/*                                                                         */
/* Use, modification and distribution is subject to the Boost Software     */
/* License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at */
/* http://www.boost.org/LICENSE_1_0.txt)                                   */
/*                                                                         */
/*  Authors: Jeremiah Willcock                                             */
/*           Andrew Lumsdaine                                              */

#include "common.h"
#include "oned_csr.h"
#include <mpi.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

#include <Grappa.hpp>
#include <GlobalCompletionEvent.hpp>
#include <Collective.hpp>
#include <Delegate.hpp>
#include <AsyncDelegate.hpp>
#include <SharedMessagePool.hpp>


static oned_csr_graph g;
static int64_t* g_oldq;
static int64_t* g_newq;
static unsigned long* g_visited;
static const int coalescing_size = 256;
static int64_t* g_outgoing;
static size_t* g_outgoing_counts /* 2x actual count */;
static MPI_Request* g_outgoing_reqs;
static int* g_outgoing_reqs_active;
static int64_t* g_recvbuf;

static size_t oldq_count = 0;
static size_t newq_count = 0;

static int64_t * g_pred = NULL;

static Grappa::GlobalCompletionEvent mygce;

void make_graph_data_structure(const tuple_graph* const tg) {
  convert_graph_to_oned_csr(tg, &g);
  const size_t nlocalverts = g.nlocalverts;
  g_oldq = (int64_t*)xmalloc(nlocalverts * sizeof(int64_t));
  g_newq = (int64_t*)xmalloc(nlocalverts * sizeof(int64_t));
  const int ulong_bits = sizeof(unsigned long) * CHAR_BIT;
  int64_t visited_size = (nlocalverts + ulong_bits - 1) / ulong_bits;
  g_visited = (unsigned long*)xmalloc(visited_size * sizeof(unsigned long));
  g_outgoing = (int64_t*)xMPI_Alloc_mem(coalescing_size * size * 2 * sizeof(int64_t));
  g_outgoing_counts = (size_t*)xmalloc(size * sizeof(size_t)) /* 2x actual count */;
  g_outgoing_reqs = (MPI_Request*)xmalloc(size * sizeof(MPI_Request));
  g_outgoing_reqs_active = (int*)xmalloc(size * sizeof(int));
  g_recvbuf = (int64_t*)xMPI_Alloc_mem(coalescing_size * 2 * sizeof(int64_t));
}

void free_graph_data_structure(void) {
  free(g_oldq);
  free(g_newq);
  free(g_visited);
  MPI_Free_mem(g_outgoing);
  free(g_outgoing_counts);
  free(g_outgoing_reqs);
  free(g_outgoing_reqs_active);
  MPI_Free_mem(g_recvbuf);
  free_oned_csr_graph(&g);
}

int bfs_writes_depth_map(void) {
  return 0;
}

/* This version is the traditional level-synchronized BFS using two queues.  A
 * bitmap is used to indicate which vertices have been visited.  Messages are
 * sent and processed asynchronously throughout the code to hopefully overlap
 * communication with computation. */
void run_bfs(int64_t root, int64_t* pred) {
  const size_t nlocalverts = g.nlocalverts;
  g_pred = pred;

  /* Set up the queues. */
/*   int64_t* restrict oldq = g_oldq; */
/*   int64_t* restrict newq = g_newq; */
/*   size_t oldq_count = 0; */
/*   size_t newq_count = 0; */
  oldq_count = 0;
  newq_count = 0;

  /* Set up the visited bitmap. */
  const int ulong_bits = sizeof(unsigned long) * CHAR_BIT;
  int64_t visited_size = (nlocalverts + ulong_bits - 1) / ulong_bits;
  unsigned long* restrict visited = g_visited;
  memset(visited, 0, visited_size * sizeof(unsigned long));
#define SET_VISITED(v) do {visited[VERTEX_LOCAL((v)) / ulong_bits] |= (1UL << (VERTEX_LOCAL((v)) % ulong_bits));} while (0)
#define TEST_VISITED(v) ((visited[VERTEX_LOCAL((v)) / ulong_bits] & (1UL << (VERTEX_LOCAL((v)) % ulong_bits))) != 0)

  /* Set all vertices to "not visited." */
  {size_t i; for (i = 0; i < nlocalverts; ++i) pred[i] = -1;}

  /* Mark the root and put it into the queue. */
  if (VERTEX_OWNER(root) == rank) {
    SET_VISITED(root);
    g_pred[VERTEX_LOCAL(root)] = root;
    g_oldq[oldq_count++] = root;
  }

  while (1) {
    /* Step through the current level's queue. */
    mygce.reset();
    mygce.enroll(oldq_count);
    Grappa_barrier_suspending();

    size_t i;
    for (i = 0; i < oldq_count; ++i) {
      assert (VERTEX_OWNER(g_oldq[i]) == rank);
      assert (g_pred[VERTEX_LOCAL(g_oldq[i])] >= 0 && g_pred[VERTEX_LOCAL(g_oldq[i])] < g.nglobalverts);
      int64_t src = g_oldq[i];
      /* Iterate through its incident edges. */
      size_t j, j_end = g.rowstarts[VERTEX_LOCAL(g_oldq[i]) + 1];
      for (j = g.rowstarts[VERTEX_LOCAL(g_oldq[i])]; j < j_end; ++j) {
        int64_t tgt = g.column[j];
        int owner = VERTEX_OWNER(tgt);
        /* If the other endpoint is mine, update the visited map, predecessor
         * map, and next-level queue locally; otherwise, send the target and
         * the current vertex (its possible predecessor) to the target's owner.
         * */
/*         if (owner == rank) { */
/*           if (!TEST_VISITED(tgt)) { */
/*             SET_VISITED(tgt); */
/*             pred[VERTEX_LOCAL(tgt)] = src; */
/*             g_newq[newq_count++] = tgt; */
/*           } */
/*         } else { */
          Grappa::delegate::call_async<&mygce>( *Grappa::shared_pool, owner, [src,tgt] {
                                                  /* Set up the visited bitmap. */
                                                  const int ulong_bits = sizeof(unsigned long) * CHAR_BIT;
                                                  int64_t visited_size = (g.nlocalverts + ulong_bits - 1) / ulong_bits;
                                                  unsigned long* restrict visited = g_visited;

                                                  assert (VERTEX_OWNER(tgt) == rank);
                                                  if (!TEST_VISITED(tgt)) {
                                                    SET_VISITED(tgt);
                                                    g_pred[VERTEX_LOCAL(tgt)] = src;
                                                    g_newq[newq_count++] = tgt;
                                                  }
                                                } );
/*         } */
      }
      mygce.complete(1);
    }

    mygce.wait();

    /* Test globally if all queues are empty. */
    size_t global_newq_count = Grappa::allreduce< size_t, collective_add >(newq_count);

    /* Quit if they all are empty. */
    if (global_newq_count == 0) break;

    /* Swap old and new queues; clear new queue for next level. */
    {int64_t* temp = g_oldq; g_oldq = g_newq; g_newq = temp;}
    oldq_count = newq_count;
    newq_count = 0;
  }

}

void get_vertex_distribution_for_pred(size_t count, const int64_t* vertex_p, int* owner_p, size_t* local_p) {
  const int64_t* restrict vertex = vertex_p;
  int* restrict owner = owner_p;
  size_t* restrict local = local_p;
  ptrdiff_t i;
#pragma omp parallel for
  for (i = 0; i < (ptrdiff_t)count; ++i) {
    owner[i] = VERTEX_OWNER(vertex[i]);
    local[i] = VERTEX_LOCAL(vertex[i]);
  }
}

int64_t vertex_to_global_for_pred(int v_rank, size_t v_local) {
  return VERTEX_TO_GLOBAL(v_rank, v_local);
}

size_t get_nlocalverts_for_pred(void) {
  return g.nlocalverts;
}
