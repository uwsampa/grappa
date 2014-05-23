#include <Grappa.hpp>
#include <GlobalVector.hpp>
#include <graph/Graph.hpp>

#include "sssp.hpp"

/* Options */
DEFINE_bool(metrics, false, "Dump metrics");
DEFINE_int32(scale, 10, "Log2 number of vertices.");
DEFINE_int32(edgefactor, 16, "Average number of edges per vertex.");
DEFINE_int64(root, 16, "Average number of edges per vertex.");

using namespace Grappa;

int64_t nedge_traversed;

GRAPPA_DEFINE_METRIC(SummarizingMetric<double>, sssp_mteps, 0);
GRAPPA_DEFINE_METRIC(SummarizingMetric<double>, sssp_time, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<int64_t>, sssp_nedge, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, graph_create_time, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, verify_time, 0);

void dump_sssp_graph(GlobalAddress<G> &g);

// global completion flag 
bool global_complete = false;
// local completion flag
bool local_complete = false;

void do_sssp(GlobalAddress<G> &g, int64_t root) {

    // intialize parent to -1
    forall(g, [](G::Vertex& v){ v->init(v.nadj); });

    VLOG(1) << "root => " << root;

    // set zero value for root distance and
    // setup 'root' as the parent of itself
    delegate::call(g->vs+root,[=](G::Vertex& v) { 
      v->dist = 0.0;
      v->parent = root;
    });

    // expose global completion flag to global address space
    GlobalAddress<bool> complete_addr = make_global(&global_complete);

    int iter = 0;
    while (!global_complete) {
      VLOG(1) << "iteration --> " << iter++;

      global_complete = true;
      on_all_cores([]{ local_complete = true; });

      // iterate over all vertices of the graph
      forall(g, [=](VertexID vsid, G::Vertex& vs) {

        if (vs->dist !=  std::numeric_limits<double>::max()) {

          // if vertex is visited (i.e. dist != max()) then
          // visit all the adjacencies of the vertex 
          // and update there dist values if needed
          double dist = vs->dist;
          
          forall(adj(g,vs), [=](G::Edge& e){
            // calculate potentinal new distance and...
            double sum = dist + e->weight;
            // ...send it to the core where the vertex is located
            delegate::call<async>(e.ga, [=](G::Vertex& ve){
              if (sum < ve->dist) {
                // update vertex parameters
                ve->dist = sum;
                ve->parent = vsid;

                // update local global_complete flag
                local_complete = false;
              }
            });
          });//forall_here
        }//if
      });//forall

      // find if SSSP calculation is completed (must be completed
      // in each core)
      global_complete = reduce<bool,collective_and>(&local_complete);

    }//while
}

int main(int argc, char* argv[]) {
  Grappa::init(&argc, &argv);
  Grappa::run([]{
    int64_t NE = (1L << FLAGS_scale) * FLAGS_edgefactor;
    bool verified = false;
    double t;
    
    t = walltime();

    // generate "NE" edge tuples, sampling vertices using the
    // Graph500 Kronecker generator to get a power-law graph
    auto tg = TupleGraph::Kronecker(FLAGS_scale, NE, 111, 222);

    // create graph with incorporated Vertex
    auto g = G::Undirected( tg );
    graph_create_time = (walltime()-t);
    
    LOG(INFO) << "graph generated (#nodes = " << g->nv << "), " << graph_create_time;
      
    t = walltime();

    auto root = FLAGS_root;
    do_sssp(g, root);

    double this_sssp_time = walltime() - t;
    LOG(INFO) << "(root=" << root << ", time=" << this_sssp_time << ")";
    sssp_time += this_sssp_time;

    if (!verified) {
      // only verify the first one to save time
      t = walltime();
      sssp_nedge = Verificator<G>::verify(tg, g, root);
      verify_time = (walltime()-t);
      LOG(INFO) << verify_time;
      verified = true;
    }
    sssp_mteps += sssp_nedge / this_sssp_time / 1.0e6;

    LOG(INFO) << "\n" << sssp_nedge << "\n" << sssp_time << "\n" << sssp_mteps;

    if (FLAGS_metrics) Metrics::merge_and_print();
    Metrics::merge_and_dump_to_file();

    // dump graph after computation
    //dump_sssp_graph(g);

    tg.destroy();
    g->destroy();

  });
  Grappa::finalize();
}
