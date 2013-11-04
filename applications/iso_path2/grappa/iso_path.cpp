#include <Grappa.hpp>
#include <GlobalAllocator.hpp>
#include <Delegate.hpp>
#include <PerformanceTools.hpp>
#include <Statistics.hpp>
#include <Collective.hpp>

#define _GRAPPA 1
#include "../../graph500/generator/make_graph.h"

#include "../../graph500/generator/utils.h"
#include "../prng.h"
#include "common.h"

#include "timer.h"
#include "options.h"
#include "graph.hpp"
#include "verify.hpp"

#include <stack>
#include <vector>

using namespace Grappa;

/////////////////////////////////
// Declarations
double make_bfs_tree(GlobalAddress<Graph<VertexP>> g_in, GlobalAddress<int64_t> _bfs_tree, int64_t root);
long cc_benchmark(GlobalAddress<Graph<>> g);

/////////////////////////////////
// Globals

static double generation_time;
static double construction_time;
static double bfs_time[NBFS_max];
static int64_t bfs_nedge[NBFS_max];

DEFINE_int64(scale, 3, "Graph500 scale (graph will have ~2^scale vertices)");
DEFINE_int64(edgefactor, 1, "Approximate number of edges in graph will be 2*2^(scale)*edgefactor");
DEFINE_int64(nbfs, 8, "Number of BFS traversals to do");

DEFINE_string(bench, "bfs", "Specify what graph benchmark to execute.");

DEFINE_bool(verify, true, "Do verification. Note: `--noverify` is equivalent to `--verify=(false|no|0)`");

DEFINE_double(beamer_alpha, 20.0, "Beamer BFS parameter for switching to bottom-up.");
DEFINE_double(beamer_beta, 20.0, "Beamer BFS parameter for switching back to top-down.");


//#define TRACE 0 //outputs traversal trace if high

//-----[DELEGATE CALL HELPERS]-----//

//delegate call to read color
int64_t read_color(GlobalAddress<VertexP> v) {
  return delegate::call(v, [](VertexP *v) {return v->parent(); });
}

//delegate call to write color
void write_color(GlobalAddress<VertexP> v, int64_t c) {
  return delegate::call(v, [c](VertexP * v) {v->parent(c); });
}

//#define TRACE 1

int find_iso_paths(GlobalAddress<Graph<VertexP>> g, int64_t node, std::stack<int64_t> &pattern, std::vector<int64_t> &visited_nodes) {

  if (pattern.size() == 0) {
    return 0;
  }
  int64_t color = pattern.top();
  int64_t vcolor = (int64_t) (g->vs+node)->vertex_data;

  //delegate read call to get node color
  vcolor = read_color(g->vs+node);
  
#ifdef TRACE
  LOG(INFO) << "Traversed node: " << node << " with color " << (int64_t) (g->vs+node)->vertex_data << " patc = " << color << " vtxc = " << vcolor << " pattern depth: " << pattern.size();
#endif

  if (color == vcolor) {
    if (pattern.size() == 1) {
      LOG(INFO) << "Found path ending at node: " << node << " - Pattern depth: " << pattern.size();
      return 1; //end of path, return found 1 path
    }  
    else { //recursive case
      int paths = 0;
      
      for (int64_t i = 0; i < (g->vs+node)->nadj; i++) {
	//add the current node to the list of visited nodes
	visited_nodes.push_back(node);

	//get the next node in the adjacency array
	int64_t next_node = (g->vs+node)->local_adj[i];
	
	//if not visited already or current node, traverse
	if(std::find(visited_nodes.begin(), visited_nodes.end(), next_node) == visited_nodes.end()) {
	  pattern.pop();
	  paths += find_iso_paths(g, next_node, pattern, visited_nodes);
	  pattern.push(color);
	}
	
	//remove the current node from list of visited nodes
	visited_nodes.pop_back();
      }
      return paths;
    }
  } else {
    return 0;
  }

  return 0;
}


//applies delegate call to write colors to nodes
//TODO: make this a forall_localized call
void set_color(GlobalAddress<Graph<VertexP>> g) {
  int64_t num_vtx = g->nv;
  for (int64_t n = 0; n < num_vtx; n++) {
    write_color(g->vs+n, 1);
    //(g->vs+n)->parent(n);
  }
}

void set_color2(GlobalAddress<Graph<VertexP>> g) {
  int64_t num_vtx = g->nv;
  for (int64_t n = 0; n < num_vtx; n++) {
    int color = n % 2;
    write_color(g->vs+n, color);
    
    //(g->vs+n)->parent(n % 2);
  }
}

std::stack<int64_t> generate_pattern(std::vector<int64_t> pattern) {
  std::stack<int64_t> p;
  for (int64_t i = pattern.size() - 1; i >= 0; i++) {
    p.push(pattern[i]);
  }
  return p;
}

//grappa version of the isomorphic path implementation
void grappa_iso_paths(tuple_graph &tg, GlobalAddress<Graph<>> generic_graph) {
  //set the vertices to have color 1
  auto g = Graph<>::transform_vertices<VertexP>(generic_graph, [](VertexP & v) { v.parent(1); });
  set_color(g);
  //set_color2(g);
  Graph<VertexP>::dump(g);
  
  //forall_local
  int grappa_paths = 0;
  int seq_paths = 0;
  int paths = 0;

  //holder for path results
  LOG(INFO) << "Allocating " << g->nv << " result slots...\n";
  GlobalAddress<int64_t> results = global_alloc<int64_t>(g->nv);
  
  //GRAPPA isopaths call
  forall_localized(g->vs, g->nv, [g, results] (int64_t i, VertexP &v) {
      //generate a node copy of the visited nodes list
      std::vector<int64_t> visited_nodes;
      
      //generate a node copy of the pattern
      //TOFIX: this is a hack
      std::stack<int64_t> pattern;
      for (int j = 0; j < 3; j++) {
	pattern.push((int64_t) 1);
      }

      //run the path search on the node
      #ifdef TRACE
      LOG(INFO) << "Starting path search at node: " << i; 
      #endif
      int num_paths = find_iso_paths(g, i , pattern, visited_nodes);
      #ifdef TRACE
      LOG(INFO) << "Number of paths from node " << i << ": " << num_paths;
      #endif
      delegate::write(results+i, num_paths);
    });

  //collect the result path count
  for (int j = 0; j < g->nv; j++) {
    grappa_paths += delegate::read(results+j);
  }

  LOG(INFO) << "[Grappa] Total number of isomorphic paths: " << grappa_paths;
}


//reference implementation of the isomorphic path implementation
void ref_iso_paths(tuple_graph &tg, GlobalAddress<Graph<>> generic_graph) {
  auto g = Graph<>::transform_vertices<VertexP>(generic_graph, [](VertexP & v) { v.parent(1); });
  //set_color(g);
  set_color2(g);
  Graph<VertexP>::dump(g);
  
  std::stack<int64_t> pattern;

  for (int j = 0; j < 2; j++) {
    pattern.push((int64_t) 1);
  }

  int paths = 0;
  int seq_paths = 0;

  //iterate over all vertices
  for (int64_t n = 0; n < g->nv; n++) {
    std::vector<int64_t> visited_nodes;
    #ifdef TRACE
    LOG(INFO) << "Starting path search at node: " << n;
    #endif
    paths = find_iso_paths(g, n, pattern, visited_nodes);
    #ifdef TRACE
    LOG(INFO) << "Number of paths from node: " << n << " = " << paths;
    #endif
    seq_paths += paths;
  }
  LOG(INFO) << "[Sequential] Total number of isomorphic paths: " << seq_paths;
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
  
  //run the isomorphics paths routine
  grappa_iso_paths(tg, g);

  //run the sequential reference version of isomorphic paths routine
  //ref_iso_paths(tg, g);

  // choose benchmark
  if (FLAGS_bench.find("bfs") != std::string::npos) {
    //bfs_benchmark(tg, g, FLAGS_nbfs);
  }
  if (FLAGS_bench.find("cc") != std::string::npos) {

  }
  
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
