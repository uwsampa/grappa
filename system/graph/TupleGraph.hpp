#include <Addressing.hpp>
#include <GlobalAllocator.hpp>

namespace Grappa {

  class TupleGraph {
  private:
    bool initialized;

  public:    
    struct Edge { int64_t v0, v1; };


    GlobalAddress<Edge> edges;
    int64_t nedge; /* Number of edges in graph, in both cases */
  
    /// Use Graph500 Kronecker generator (@see graph/KroneckerGenerator.cpp)
    static TupleGraph Kronecker(int scale, int64_t desired_nedge, 
                                         uint64_t seed1, uint64_t seed2);
    
     void destroy() { global_free(edges); }

    // default contstructor
    TupleGraph()
      : initialized( false )
      , edges( )
      , nedge(0)
    { }

    TupleGraph(const TupleGraph& tg): initialized(false), edges(tg.edges), nedge(tg.nedge) { }

    TupleGraph& operator=(const TupleGraph& tg) {
      if( initialized ) {
        global_free<Edge>(edges);
      }
      edges = tg.edges;
      nedge = tg.nedge;
      return *this;
    }
    
  protected:
    TupleGraph(int64_t nedge): initialized(true), edges(global_alloc<Edge>(nedge)), nedge(nedge) {}
    
  };

}
