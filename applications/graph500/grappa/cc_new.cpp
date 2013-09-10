#include "graph.hpp"
using namespace Grappa;
namespace d = Grappa::delegate;

DECLARE_int64(cc_hash_size);
DEFINE_int64(cc_concurrent_roots, 1, "number of concurrent `explores` to have at a given time");
DEFINE_bool(cc_insert_async, false, "do inserts asynchronously (keeps them local until end)");

struct Edge {
  graphint start, end;
  bool operator==(const Edge& e) const { return e.start == start && e.end == end; }
  friend std::ostream& operator<<(std::ostream& o, const Edge& e);
};
std::ostream& operator<<(std::ostream& o, const Edge& e) {
  o << "{" << e.start << "," << e.end << "}";
  return o;
}
namespace std {
  template <> struct hash<Edge> {
    typedef size_t result_type;
    typedef Edge argument_type;
    size_t operator()(Edge const & e) const noexcept {
      return static_cast<size_t>(e.start ^ e.end);
    }
  };
}

///////////////////////////////////////////////////////////////
// Globals
static GlobalAddress<Graph<VertexP>> g;
static GlobalAddress<graphint> colors;
static GlobalAddress<graphint> visited;
static GlobalAddress<color_t> marks;
static bool changed;
static graphint ncomponents;

static GlobalAddress<GlobalHashSet<Edge>> component_edges;


void pram_cc(GlobalAddress<GlobalHashSet<Edge>> component_edges, long NV) {
  int npass = 0;
  do {
    DVLOG(2) << "npass " << npass;
    call_on_all_cores([]{ changed = false; });
    
    // Hook
    DVLOG(2) << "hook";
    component_edges->forall_keys([NV](Edge& e){
      graphint i = e.start, j = e.end;
      CHECK_LT(i, NV);
      CHECK_LT(j, NV);
      graphint ci = d::read(colors+e.start),
               cj = d::read(colors+e.end);
      CHECK_LT(ci, NV);
      CHECK_LT(cj, NV);
      bool lchanged = false;
    
      if ( ci < cj ) {
        lchanged = lchanged || d::call(colors+cj, [ci,cj](graphint* ccj){
          if (*ccj == cj) { *ccj = ci; return true; }
          else { return false; }
        });
      }
      if (!lchanged && cj < ci) {
        lchanged = lchanged || d::call(colors+ci, [ci,cj](graphint* cci){
          if (*cci == ci) { *cci = cj; return true; }
          else { return false; }
        });
      }
    
      if (lchanged) { changed = true; }
    });
    
    // Compress
    DVLOG(2) << "compress";
    component_edges->forall_keys([NV](Edge& e){
      auto compress = [NV](graphint i) {
        graphint ci, cci, nc;
        ci = nc = d::read(colors+i);
        CHECK_LT(ci, NV);
        CHECK_LT(nc, NV);
        while ( nc != (cci=d::read(colors+nc)) ) { nc = d::read(colors+cci); CHECK_LT(nc, NV); }
        if (nc != ci) {
          changed = true;
          d::write(colors+i, nc);
        }
      };
      
      compress(e.start);
      compress(e.end);
    });
    npass++;
  } while (reduce<bool,collective_or>(&changed) == true);
  
  LOG(INFO) << "cc_pram_passes: " << npass;
}

long cc_benchmark(GlobalAddress<Graph<>> in) {
  call_on_all_cores([in]{
    
  });
  return 0;
}