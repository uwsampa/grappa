#include <Grappa.hpp>
#include <GlobalAllocator.hpp>
#include <Delegate.hpp>
#include <PerformanceTools.hpp>
#include <Statistics.hpp>
#include <Collective.hpp>

#define _GRAPPA 1
#include "../generator/make_graph.h"

#include "../generator/utils.h"
#include "../prng.h"
#include "common.h"

#include "timer.h"
#include "options.h"
#include "graph.hpp"
#include "verify.hpp"

using namespace Grappa;

/////////////////////////////////
// Declarations
double make_bfs_tree(GlobalAddress<Graph<VertexP>> g_in, GlobalAddress<int64_t> _bfs_tree, int64_t root);
long cc_benchmark(GlobalAddress<Graph<>> g);

/////////////////////////////////
// Options/flags

DEFINE_int64(scale, 16, "Graph500 scale (graph will have ~2^scale vertices)");
DEFINE_int64(edgefactor, 16, "Approximate number of edges in graph will be 2*2^(scale)*edgefactor");
DEFINE_int64(nbfs, 8, "Number of BFS traversals to do");

DEFINE_string(bench, "bfs", "Specify what graph benchmark to execute.");

DEFINE_bool(verify, true, "Do verification. Note: `--noverify` is equivalent to `--verify=(false|no|0)`");

DEFINE_double(beamer_alpha, 20.0, "Beamer BFS parameter for switching to bottom-up.");
DEFINE_double(beamer_beta, 20.0, "Beamer BFS parameter for switching back to top-down.");

/////////////////////////////////
// Globals

static double generation_time;
static double construction_time;
static double bfs_time[NBFS_max];
static int64_t bfs_nedge[NBFS_max];

template< class V >
static void choose_bfs_roots(GlobalAddress<Graph<V>> g, int * nbfs, int64_t bfs_roots[]) {
  auto has_adj = [g](int64_t i) {
    return delegate::call(g->vs+i, [](V * v){
      return v->nadj > 0;
    });
  };
  
  // sample from 0..nvtx-1 without replacement
  int64_t m = 0, t = 0;
  while (m < *nbfs && t < g->nv) {
    double R = mrg_get_double_orig((mrg_state*)prng_state);
    if (!has_adj(t) || (g->nv - t)*R > *nbfs - m) ++t;
    else bfs_roots[m++] = t++;
  }
  if (t >= g->nv && m < *nbfs) {
    if (m > 0) {
      LOG(INFO) << "Cannot find enough sample roots, using " << m;
      *nbfs = m;
    } else {
      LOG(ERROR) << "Cannot find any sample roots! Exiting...";
      exit(1);
    }
  }
}

void bfs_benchmark(tuple_graph& tg, GlobalAddress<Graph<>> generic_graph, int nroots) {
  auto g = Graph<>::transform_vertices<VertexP>(generic_graph, [](VertexP& v){ v.parent(-1); });
  GlobalAddress<int64_t> bfs_tree = global_alloc<int64_t>(g->nv);
  
  int64_t bfs_roots[NBFS_max];
  choose_bfs_roots(g, &nroots, bfs_roots);
  
  // build bfs tree for each root
  for (int64_t i=0; i < nroots; i++) {
    Grappa::memset(bfs_tree, -1, g->nv);
    
    if (i == 0) Statistics::start_tracing();
    
    bfs_time[i] = make_bfs_tree(g, bfs_tree, bfs_roots[i]);

    if (i == 0) Statistics::stop_tracing();
    
    VLOG(1) << "make_bfs_tree time: " << bfs_time[i];
    
    if (FLAGS_verify) {
      VLOG(1) << "Verifying bfs " << i << "...";
      
      // GRAPPA_TIME_LOG("verify_time") {
        bfs_nedge[i] = verify_bfs_tree(bfs_tree, g->nv-1, bfs_roots[i], &tg);
      // }
      
      if (bfs_nedge[i] < 0) {
        LOG(ERROR) << "bfs " << i << " from root " << bfs_roots[i] << " failed verification: " << bfs_nedge[i];
        exit(1);
      } else {
        VLOG(0) << "bfs_time[" << i << "] = " << bfs_time[i];
      }
    } else {
      // LOG(ERROR) << "warning: skipping verify. Approximating nedge with `nadj/2`.";
      bfs_nedge[i] = g->nadj/2;
    }
  }
  
  global_free(bfs_tree);
  /* Print results. */
  output_results(FLAGS_scale, 1<<FLAGS_scale, FLAGS_edgefactor, A, B, C, D, generation_time, construction_time, nroots, bfs_time, bfs_nedge);
  Statistics::merge_and_print();
}

void user_main(void * ignore) {
  double t, start_time;
  start_time = walltime();
  
	int64_t nvtx_scale = ((int64_t)1)<<FLAGS_scale;
	int64_t desired_nedge = nvtx_scale * FLAGS_edgefactor;
  
  LOG(INFO) << "scale = " << FLAGS_scale << ", NV = " << nvtx_scale << ", NE = " << desired_nedge;
  
  // make raw graph edge tuples
  tuple_graph tg;
  tg.edges = global_alloc<packed_edge>(desired_nedge);
  
  t = walltime();
  make_graph( FLAGS_scale, desired_nedge, userseed, userseed, &tg.nedge, &tg.edges );
  generation_time = walltime() - t;
  LOG(INFO) << "graph_generation: " << generation_time;
  
  t = walltime();
  auto g = Graph<>::create(tg);
  construction_time = walltime() - t;
  LOG(INFO) << "construction_time: " << construction_time;
  
  // Graph::dump(g);
  
  // choose benchmark
  if (FLAGS_bench.find("bfs") != std::string::npos) {
    bfs_benchmark(tg, g, FLAGS_nbfs);
  }
  if (FLAGS_bench.find("cc") != std::string::npos) {
    long ncomponents = cc_benchmark(g);
  }
  
  // Grappa::Statistics::merge_and_print(std::cout);
  
  g->destroy();
  global_free(tg.edges);
    
  LOG(INFO) << "total_runtime: " << walltime() - start_time;
}

int main(int argc, char** argv) {
  Grappa_init(&argc, &argv);
  Grappa_activate();

  Grappa_run_user_main(&user_main, (void*)NULL);
  
  Grappa_finish(0);
  return 0;
}
