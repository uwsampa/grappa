/* Copyright (C) 2010 The Trustees of Indiana University.                  */
/*                                                                         */
/* Use, modification and distribution is subject to the Boost Software     */
/* License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at */
/* http://www.boost.org/LICENSE_1_0.txt)                                   */
/*                                                                         */
/*  Authors: Jeremiah Willcock                                             */
/*           Andrew Lumsdaine                                              */

/* These need to be before any possible inclusions of stdint.h or inttypes.h.
 * */
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include "../generator/make_graph.h"
#include "../generator/utils.h"
#include "../prng.h"
#include "common.h"
#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <limits.h>
#include <stdint.h>
#include <inttypes.h>

#include <Grappa.hpp>
#include <GlobalAllocator.hpp>
#include <Cache.hpp>
#include <Delegate.hpp>
#include <PerformanceTools.hpp>
#include <FileIO.hpp>
#include <Statistics.hpp>
#include <Collective.hpp>

#include "timer.h"
//#include "rmat.h"
#include "oned_csr.h"
#include "verify.hpp"
#include "options.h"

using namespace Grappa;

GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, bfs_vertex_visited, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, bfs_edge_visited, 0);

DEFINE_double(beamer_alpha, 20.0, "Beamer BFS parameter for switching to bottom-up.");
DEFINE_double(beamer_beta, 20.0, "Beamer BFS parameter for switching back to top-down.");

// test change

//### Globals ###
static int64_t nvtx_scale;

//static int64_t bfs_root[NBFS_max];

static double generation_time;
static double construction_time;
static double bfs_time[NBFS_max];
static int64_t bfs_nedge[NBFS_max];

#define XOFF(k) (xoff+2*(k))
#define XENDOFF(k) (xoff+2*(k)+1)

inline bool has_adj(GlobalAddress<int64_t> xoff, int64_t i) {
  int64_t xoi = delegate::read(XOFF(i));
  int64_t xei = delegate::read(XENDOFF(i));
  return xei-xoi != 0;
}

static void choose_bfs_roots(GlobalAddress<int64_t> xoff, int64_t nvtx, int * NBFS, int64_t bfs_roots[]) {

  // sample from 0..nvtx-1 without replacement
  int64_t m = 0, t = 0;
  while (m < *NBFS && t < nvtx) {
    double R = mrg_get_double_orig((mrg_state*)prng_state);
    if (!has_adj(xoff, t) || (nvtx - t)*R > *NBFS - m) ++t;
    else bfs_roots[m++] = t++;
  }
  if (t >= nvtx && m < *NBFS) {
    if (m > 0) {
      LOG(INFO) << "Cannot find enough sample roots, using " << m;
      *NBFS = m;
    } else {
      LOG(ERROR) << "Cannot find any sample roots! Exiting...";
      exit(1);
    }
  }
}

static void run_bfs(tuple_graph * tg, csr_graph * g, int64_t * bfs_roots) {
  double t;
  
  // build bfs tree for each root
  for (int64_t i=0; i < NBFS; i++) {
    GlobalAddress<int64_t> bfs_tree = Grappa::global_alloc<int64_t>(g->nv);
    GlobalAddress<int64_t> max_bfsvtx;
    
    VLOG(1) << "Running bfs on root " << i << "(" << bfs_roots[i] << ")...";
    // call_on_all_cores([]{ Statistics::reset(); });

    Statistics::start_tracing();

    t = timer();
    bfs_time[i] = make_bfs_tree(g, bfs_tree, bfs_roots[i]);
    t = timer() - t;

    Statistics::stop_tracing();
    
    VLOG(1) << "make_bfs_tree time: " << t;

    VLOG(1) << "Verifying bfs " << i << "...";
    t = timer();
    bfs_nedge[i] = verify_bfs_tree(bfs_tree, g->nv-1, bfs_roots[i], tg);
    t = timer() - t;
    VLOG(1) << "verify time: " << t;
    
    if (bfs_nedge[i] < 0) {
      LOG(ERROR) << "bfs " << i << " from root " << bfs_roots[i] << " failed verification: " << bfs_nedge[i];
      exit(1);
    } else {
      VLOG(1) << "bfs_time[" << i << "] = " << bfs_time[i];
    }
    
    TIME(t, Grappa::global_free(bfs_tree));
    VLOG(1) << "Free bfs_tree time: " << t;
  }
}

static void checkpoint_in(tuple_graph * tg, csr_graph * g, int64_t * bfs_roots) {
  //TAU_PHASE("checkpoint_in","void (tuple_graph*,csr_graph*,int64_t*)", TAU_USER);
  bool agg_enable = FLAGS_aggregator_enable;
  FLAGS_aggregator_enable = true;

  VLOG(1) << "start reading checkpoint";
  double t = timer();
  
  char fname[256];
  // sprintf(fname, "ckpts/graph500.%lld.%lld.xmt.w.ckpt", SCALE, edgefactor);
  sprintf(fname, "ckpts/graph500.%lld.%lld.grappa.ckpt", SCALE, edgefactor);
  FILE * fin = fopen(fname, "r");
  if (!fin) {
    LOG(ERROR) << "Unable to open file: " << fname << ", will generate graph and write checkpoint.";
    load_checkpoint = false;
    write_checkpoint = true;
    return;
  }
  
  int64_t ckpt_nbfs;

  fread(&tg->nedge, sizeof(tg->nedge), 1, fin);
  fread(&g->nv, sizeof(g->nv), 1, fin);
  fread(&g->nadj, sizeof(g->nadj), 1, fin);
  fread(&ckpt_nbfs, sizeof(ckpt_nbfs), 1, fin);
  CHECK(ckpt_nbfs <= NBFS_max);

  tg->edges = Grappa::global_alloc<packed_edge>(tg->nedge);
  g->xoff = Grappa::global_alloc<int64_t>(2*g->nv+2);
  g->xadjstore = Grappa::global_alloc<int64_t>(g->nadj);
  g->xadj = g->xadjstore+2;
  
  Grappa::File gfin(fname, false);
  gfin.offset = 4*sizeof(int64_t);

  Grappa::read_array(gfin, tg->edges, tg->nedge);
  
  Grappa::read_array(gfin, g->xoff, 2*g->nv+2);

  Grappa::read_array(gfin, g->xadjstore, g->nadj);
  
  fseek(fin, gfin.offset, SEEK_SET);
  fread(bfs_roots, sizeof(int64_t), ckpt_nbfs, fin);
  fclose(fin);
 
  if (ckpt_nbfs == 0) {
    fprintf(stderr, "no bfs roots found in checkpoint, generating on the fly\n");
  } else if (ckpt_nbfs < NBFS) {
    fprintf(stderr, "only %" PRId64 " bfs roots found\n", ckpt_nbfs);
    NBFS = ckpt_nbfs;
  }

  int64_t total_size = (2*g->nv+2 + tg->nedge + g->nadj)*sizeof(int64_t);
  t = timer() - t;
  VLOG(1) << "checkpoint_read_time: " << t;
  VLOG(1) << "checkpoint_read_rate: " << total_size / t / (1L<<20);
  FLAGS_aggregator_enable = agg_enable;
}

static void checkpoint_out(tuple_graph * tg, csr_graph * g, int64_t * bfs_roots) {
  VLOG(1) << "starting checkpoint output";
  double t = timer();
  
  char fname[256];
  sprintf(fname, "ckpts/graph500.%lld.%lld.grappa%s.ckpt", SCALE, edgefactor, (use_RMAT)?".rmat":"");
  FILE * fout = fopen(fname, "w");
  if (!fout) {
    LOG(ERROR) << "Unable to open file for writing: " << fname;
    exit(1);
  }
  
  VLOG(2) << "nedge: " << tg->nedge << ", nv: " << g->nv << ", nadj: " << g->nadj << ", nbfs: " << NBFS;
  fwrite(&tg->nedge, sizeof(tg->nedge), 1, fout);
  fwrite(&g->nv, sizeof(g->nv), 1, fout);
  fwrite(&g->nadj, sizeof(g->nadj), 1, fout);
  int64_t nbfs_i64 = static_cast<int64_t>(NBFS);
  fwrite(&nbfs_i64, sizeof(nbfs_i64), 1, fout);
  
  // write out edge tuples
  write_array(tg->edges, tg->nedge, fout);
  
  // xoff
  write_array(g->xoff, 2*g->nv+2, fout);
  
  // xadj
  write_array(g->xadjstore, g->nadj, fout);
  
  // bfs_roots
  fwrite(bfs_roots, sizeof(int64_t), NBFS, fout);
  
  fclose(fout);
  
  t = timer() - t;
  VLOG(1) << "checkpoint_write_time: " << t;
}

static void setup_bfs(tuple_graph * tg, csr_graph * g, int64_t * bfs_roots) {
  
  TIME(construction_time,
       create_graph_from_edgelist(tg, g)
  );
  LOG(INFO) << "construction_time: " << construction_time;
  
  GlobalAddress<int64_t> xoff = g->xoff;
//  for (int64_t i=0; i < g.nv; i++) {
//    VLOG(1) << "xoff[" << i << "] = " << read(XOFF(i)) << " -> " << read(XENDOFF(i));
//  }
//#ifdef DEBUG
//  for (int64_t i=0; i<g.nv; i++) {
//    std::stringstream ss;
//    int64_t xoi = read(XOFF(i)), xei = read(XENDOFF(i));
//    CHECK(xoi <= g.nadj) << i << ": " << xoi << " > " << g.nadj;
//    CHECK(xei <= g.nadj) << i << ": " << xei << " > " << g.nadj;
//    ss << "xoff[" << i << "] = " << xoi << "->" << xei << ": (";
//    for (int64_t j=xoi; j<xei; j++) {
//      ss << read(g.xadj+j) << ",";
//    }
//    ss << ")";
//    VLOG(2) << ss.str();
//  }
//#endif
  double t;
  
  // no rootname input method, so randomly choose
  TIME(t, choose_bfs_roots(g->xoff, g->nv, &NBFS, bfs_roots));
  LOG(INFO) << "choose_bfs_roots_time: " << t;
  
//  for (int64_t i=0; i < nbfs; i++) {
//    VLOG(1) << "bfs_roots[" << i << "] = " << bfs_roots[i];
//  }
}

int main(int argc, char* argv[]) {
  Grappa::init(&argc, &argv);
  
  /* Parse arguments. */
  get_options(argc, argv);
  
  Grappa::run([]{
    double t = timer();
  
    tuple_graph tg;
    csr_graph g;
    int64_t bfs_roots[NBFS_max];

  	nvtx_scale = ((int64_t)1)<<SCALE;
  	int64_t desired_nedge = desired_nedge = nvtx_scale * edgefactor;
  	/* Catch a few possible overflows. */
  	assert (desired_nedge >= nvtx_scale);
  	assert (desired_nedge >= edgefactor);
  
    LOG(INFO) << "scale = " << SCALE << ", nv = " << nvtx_scale << ", edgefactor = " << edgefactor << ", nedge = " << desired_nedge;

    if (load_checkpoint) {
      checkpoint_in(&tg, &g, bfs_roots);
    } // checkpoint_in may change 'load_checkpoint' to false if unable to read in file correctly
  
    if (!load_checkpoint) {
      tg.edges = global_alloc<packed_edge>(desired_nedge);
    
      /* Make the raw graph edges. */
      /* Get roots for BFS runs, plus maximum vertex with non-zero degree (used by
       * validator). */
    
      double start, stop;
      start = timer();
      {
        make_graph( SCALE, desired_nedge, userseed, userseed, &tg.nedge, &tg.edges );
      }
      stop = timer();
      generation_time = stop - start;
      LOG(INFO) << "graph_generation: " << generation_time;
    
      // construct graph
      setup_bfs(&tg, &g, bfs_roots);
    
      if (write_checkpoint) checkpoint_out(&tg, &g, bfs_roots);
    }

    // watch out for profiling! check the tau 
    Grappa::Statistics::reset();
  
    run_bfs(&tg, &g, bfs_roots);
  

    Grappa::Statistics::merge_and_print(std::cout);
    fflush(stdout);
  
  //  free_graph_data_structure();
  
    Grappa::global_free(tg.edges);
  
    /* Print results. */
    output_results(SCALE, 1<<SCALE, edgefactor, A, B, C, D, generation_time, construction_time, NBFS, bfs_time, bfs_nedge);

    t = timer() - t;
    std::cout << "total_runtime: " << t << std::endl;
  
  });
  Grappa::finalize();
}
