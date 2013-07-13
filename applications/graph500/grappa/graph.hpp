#include <Communicator.hpp>
#include <Addressing.hpp>
#include <Collective.hpp>
#include <ParallelLoop.hpp>
#include <GlobalAllocator.hpp>
#include <Delegate.hpp>
#include <AsyncDelegate.hpp>
#include <Array.hpp>

#include <algorithm>

#include "common.h"

using namespace Grappa;

struct Graph {
  struct Vertex {
    int64_t * local_adj; // adjacencies that are local
    int64_t nadj;        // number of adjacencies
    int64_t local_sz;    // size of local allocation (regardless of how full it is)
    char _pad[8];           // placeholder
    
    Vertex(): local_adj(nullptr), nadj(0), local_sz(0) {}
    
    ~Vertex() {
      if (local_sz > 0) { delete[] local_adj; }
    }
    
    template< typename F >
    void forall_adj(F body) {
      for (int64_t i=0; i<nadj; i++) {
        body(local_adj[i]);
      }
    }
  };
  static_assert(block_size % sizeof(Vertex) == 0, "Vertex size not evenly divisible into blocks!");
  
  // // Helpers (for if we go with custom cyclic distribution)
  // inline Core    vertex_owner (int64_t v) { return v % cores(); }
  // inline int64_t vertex_offset(int64_t v) { return v / cores(); }
  
  // Fields
  GlobalAddress<Vertex> vs;
  int64_t nv, nadj, nadj_local;
  
  // Internal fields
  int64_t * adj_buf;
  int64_t * scratch;
  
  GlobalAddress<Graph> self;
  
  char _pad[block_size - sizeof(vs)-sizeof(nv)-sizeof(nadj)-sizeof(nadj_local)-sizeof(adj_buf)-sizeof(scratch)-sizeof(self)];
  
  Graph(GlobalAddress<Graph> self, GlobalAddress<Vertex> vs, int64_t nv)
    : self(self)
    , vs(vs)
    , nv(nv)
    , nadj(0)
    , nadj_local(0)
    , adj_buf(nullptr)
    , scratch(nullptr)
  { }
  
  ~Graph() {
    for (Vertex& v : iterate_local(vs, nv)) { v.~Vertex(); }
    if (adj_buf) free(adj_buf);
  }
  
  // Constructor
  static GlobalAddress<Graph> create(tuple_graph& tg);
  
  void destroy() {
    auto self = this->self;
    forall_localized(this->vs, this->nv, [](Vertex& v){ v.~Vertex(); });
    global_free(this->vs);
    call_on_all_cores([self]{ self->~Graph(); });
    global_free(self);
  }
  
  template< int LEVEL = 0 >
  static void dump(GlobalAddress<Graph> g) {
    for (int64_t i=0; i<g->nv; i++) {
      delegate::call(g->vs+i, [i](Vertex * v){
        VLOG(LEVEL) << "<" << i << ">" << util::array_str("", v->local_adj, v->nadj);
      });
    }
  }
  
};
