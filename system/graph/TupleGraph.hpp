#include <Addressing.hpp>
#include <GlobalAllocator.hpp>

namespace Grappa {

  class TupleGraph {
  public:    
    struct Edge { int64_t v0, v1; };

    GlobalAddress<Edge> edges;
    int64_t nedge; /* Number of edges in graph, in both cases */
  
    /// Use Graph500 Kronecker generator (@see graph/KroneckerGenerator.cpp)
    static TupleGraph Kronecker(int scale, int64_t desired_nedge, 
                                         uint64_t seed1, uint64_t seed2);
    
  protected:
    TupleGraph(int64_t nedge): edges(global_alloc<Edge>(nedge)), nedge(nedge) {}
    
  };

}
