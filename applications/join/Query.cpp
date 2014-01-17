// grappa
#include <Grappa.hpp>
#include <Collective.hpp>

// stl
#include <vector>
#include <unordered_map>

// query library
#include "relation_io.hpp"
#include "Query.hpp"

// queries
#include "squares.hpp"
#include "squares_partition.hpp"
#include "squares_bushy.hpp"
#include "squares_partition_bushy.hpp"

// graph gen
#include "generator/make_graph.h"
#include "generator/utils.h"
#include "grappa/timer.h"
#include "grappa/common.h"
#include "grappa/graph.hpp"
#include "prng.h"


// input data options
DEFINE_uint64( scale, 7, "Graph will have ~ 2^scale vertices" );
DEFINE_uint64( edgefactor, 16, "Median degree; graph will have ~ 2*edgefactor*2^scale edges" );

DEFINE_string( fin, "", "Tuple file input" );
DEFINE_uint64( finNumTuples, 0, "Number of tuples in file input" );

DEFINE_string( query, "SquareQuery", "Name of the query plan" );


// Define query-stats

// intermediate results counts
// only care about min, max, totals
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, ir1_count, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, ir2_count, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, ir3_count, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, ir4_count, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, ir5_count, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, ir6_count, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, ir7_count, 0);

//outputs
GRAPPA_DEFINE_METRIC(SummarizingMetric<uint64_t>, ir1_final_count, 0);
GRAPPA_DEFINE_METRIC(SummarizingMetric<uint64_t>, ir2_final_count, 0);
GRAPPA_DEFINE_METRIC(SummarizingMetric<uint64_t>, ir3_final_count, 0);
GRAPPA_DEFINE_METRIC(SummarizingMetric<uint64_t>, ir4_final_count, 0);
GRAPPA_DEFINE_METRIC(SummarizingMetric<uint64_t>, ir5_final_count, 0);
GRAPPA_DEFINE_METRIC(SummarizingMetric<uint64_t>, ir6_final_count, 0);
GRAPPA_DEFINE_METRIC(SummarizingMetric<uint64_t>, ir7_final_count, 0);

GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, results_count, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, query_runtime, 0);


GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, edges_transfered, 0);

GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, total_edges, 0);

GRAPPA_DEFINE_METRIC(SimpleMetric<double>, index_runtime, 0);


// calculated parameters
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, participating_cores, 0);

int main(int argc, char** argv) {
  Grappa::init(&argc, &argv);
  Grappa::run([]{
    double t, start_time;
    start_time = walltime();
  
    tuple_graph tg;

  if (FLAGS_fin.compare("") != 0) {
    tg = readEdges( FLAGS_fin, FLAGS_finNumTuples );
  } else {
    int64_t nvtx_scale = ((int64_t)1)<<FLAGS_scale;
    int64_t desired_nedge = nvtx_scale * FLAGS_edgefactor;

      LOG(INFO) << "scale = " << FLAGS_scale << ", NV = " << nvtx_scale << ", NE = " << desired_nedge;

      // make raw graph edge tuples
      tg.edges = global_alloc<packed_edge>(desired_nedge);

      LOG(INFO) << "graph generation...";
      t = walltime();
      make_graph( FLAGS_scale, desired_nedge, userseed, userseed, &tg.nedge, &tg.edges );
      double generation_time = walltime() - t;
      LOG(INFO) << "graph_generation: " << generation_time;
    }

  
  std::unordered_map<std::string, Query*> qm( {
      {"SquareQuery", new SquareQuery()}, 
      {"SquarePartition4way", new SquarePartition4way()},
      {"SquarePartitionBushy4way", new SquarePartitionBushy4way()},
      {"SquareBushyPlan", new SquareBushyPlan()}
      });

    Query& q = *(qm[FLAGS_query]);

    LOG(INFO) << "query preprocessing... (excluded from time)";
    q.preprocessing({ tg, tg, tg, tg });
  
    on_all_cores( [] { Grappa::Metrics::reset(); } );
    double start, end;
    start = Grappa::walltime(); 
    LOG(INFO) << "query execute... (included in time)";
    q.execute({ tg, tg, tg, tg });
    on_all_cores([] {
        // used to calculate max total over all cores
        ir1_final_count=ir1_count;
        ir2_final_count=ir2_count;
        ir3_final_count=ir3_count;
        ir4_final_count=ir4_count;
        ir5_final_count=ir5_count;
        ir6_final_count=ir6_count;
        ir7_final_count=ir7_count;
    });
    
  
    end = Grappa::walltime();
    query_runtime = end - start;
    Grappa::Metrics::merge_and_print();
  });
  Grappa::finalize();
}


std::string resultStr(std::vector<int64_t> v, int64_t size) {
  std::stringstream ss;
  ss << "{ ";
  for (auto i : v) {
    ss << i << " ";
  }
  ss << "}";
  if (size!=-1) ss << " size:" << size;
  return ss.str();
}
