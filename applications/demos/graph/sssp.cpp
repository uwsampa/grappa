#include <Grappa.hpp>
#include <GlobalVector.hpp>
#include <graph/Graph.hpp>

#include "verificator.hpp"

#include <sstream>

/* Options */
DEFINE_bool(metrics, false, "Dump metrics");
DEFINE_int32(scale, 10, "Log2 number of vertices.");
DEFINE_int32(edgefactor, 16, "Average number of edges per vertex.");
DEFINE_int64(root, 16, "Average number of edges per vertex.");

using namespace Grappa;

int64_t nedge_traversed;

/* Vertex specific data */
struct SSSPData {
	double dist;
	double *weights;
	
	//TODO: these field are taken from BFSData. Should SSSPData inherit BFSData?
	int64_t parent;
	int64_t level;
	bool seen;

	void init(int64_t nadj) {
		dist = std::numeric_limits<double>::max();
		weights = new double [nadj];
		for (int i=0; i<nadj; i++)
			weights[i] = drand48();

    parent = -1;
    level = 0;
    seen = false;
	}
};

using SSSPVertex = Vertex<SSSPData>;

GRAPPA_DEFINE_METRIC(SummarizingMetric<double>, sssp_mteps, 0);
GRAPPA_DEFINE_METRIC(SummarizingMetric<double>, sssp_time, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<int64_t>, sssp_nedge, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, graph_create_time, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, verify_time, 0);

void dump_sssp_graph(GlobalAddress<Graph<SSSPVertex>> &g);
int64_t verify(TupleGraph tg, GlobalAddress<Graph<SSSPVertex>> g, int64_t root);


void do_sssp(GlobalAddress<Graph<SSSPVertex>> &g, int64_t root) {
      // intialize parent to -1
      forall(g, [](SSSPVertex& v){ v->init(v.nadj); });

      VLOG(1) << "root => " << root;

	  // set zero value for root distance and
		// setup 'root' as the parent of itself
	  delegate::call(g->vs+root,[=](SSSPVertex& v) { 
		   v->dist = 0.0;
			 v->parent = root;
		});

	  // completion flag is exposed to global addressed space
	  bool complete = false;
	  GlobalAddress<bool> complete_addr = make_global(&complete);

	  while (!complete) {
		  complete = true;
		  // iterate over all vertices of the graph
		  forall(g->vs, g->nv, [g,complete_addr](int64_t parent, SSSPVertex& vs) {

			if (vs->dist !=  std::numeric_limits<double>::max()) {
			  // if vertex is visited (i.e. dist != max()) then
			  // visit all the adjacencies of the vertex 
			  // and update there dist values if needed
			  double dist = vs->dist;
			  double *weights = vs->weights;

			  forall_here(0,vs.nadj,[=](int64_t i){
				  // calculate potentinal new distance and...
				  double sum = dist + weights[i]; 				   
				  // ...send it to the core where the vertex is located
				  bool updated = delegate::call(g->vs+vs.local_adj[i], [sum,parent](SSSPVertex& ve){
					  if (sum < ve->dist) {
						  ve->dist = sum;
							ve->parent = parent;
						  return true;
					  }
					  return false;
				  });
				  // if dist has been updated send false to completion variable to 
				  // run next iteration
				  if (updated)
				  	delegate::write(complete_addr, false);
			  });//forall_here
			}//if
		  });//forall
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

		// create graph with incorporated SSSPVertex
		auto g = Graph<SSSPVertex>::create( tg );
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
			sssp_nedge = Verificator<SSSPVertex>::verify(tg, g, root);
			verify_time = (walltime()-t);
			LOG(INFO) << verify_time;
			verified = true;
		}
		sssp_mteps += sssp_nedge / this_sssp_time / 1.0e6;

		LOG(INFO) << "\n" << sssp_nedge << "\n" << sssp_time << "\n" << sssp_mteps;

		if (FLAGS_metrics) Metrics::merge_and_print();
		Metrics::merge_and_dump_to_file();

		// dump graph after computation
		dump_sssp_graph(g);

		tg.destroy();
		g->destroy();

	});
	Grappa::finalize();
}

void dump_sssp_graph(GlobalAddress<Graph<SSSPVertex>> &g) {
	for(int i=0; i < g->nv; i++) {
		delegate::call(g->vs+i,[i](SSSPVertex &v){
			VLOG(1) << "Vertex[" << i << "] -> " << v->dist;
		});
	}
}
