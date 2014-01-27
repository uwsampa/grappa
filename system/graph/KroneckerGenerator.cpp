
#include "graph_generator.h"
#include "utils.h"
#include "TupleGraph.hpp"
#include "ParallelLoop.hpp"

namespace Grappa {

  TupleGraph TupleGraph::generate_kronecker(int scale, int64_t nedge, 
                                            uint64_t seed1, uint64_t seed2) {    
    static_assert(sizeof(TupleGraph::Edge) == sizeof(packed_edge),
                  "TupleGraph::Edge doesn't match Graph500 generator's.");
    
    TupleGraph tg(nedge);
    
    on_all_cores([tg,scale,seed1,seed2]{
      auto local_base = tg.edges.localize();
      auto local_end = (tg.edges+tg.nedge).localize();
      int64_t local_n = local_end - local_base;
      
      uint_fast32_t seed[5];
      make_mrg_seed(seed1, seed2, seed);
      
      auto start = local_n * mycore();
      auto end   = start + local_n;
      auto packed_edges = reinterpret_cast<packed_edge*>(local_base);
      generate_kronecker_range(seed, scale, start, end, packed_edges);
      
    });
    
    return tg;
  }
  
}