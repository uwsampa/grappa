#include "defs.hpp"
#include <Grappa.hpp>
#include <Collective.hpp>
#include <Delegate.hpp>
#include <ParallelLoop.hpp>
#include <GlobalCompletionEvent.hpp>
#include <GlobalHashSet.hpp>

using namespace Grappa;
namespace d = Grappa::delegate;

DECLARE_int64(cc_hash_size);
GRAPPA_DEFINE_STAT(SimpleStatistic<graphint>, cc_intermediate_set_size, 0);

static graph g;

static GlobalAddress<graphint> colors;
static GlobalAddress<graphint> visited;
static GlobalAddress<color_t> marks;
static bool changed;
static graphint ncomponents;

GlobalCompletionEvent gce;

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

using EdgeHashSet = GlobalHashSet<Edge>;
static GlobalAddress<EdgeHashSet> component_edges;

template< GlobalCompletionEvent * GCE >
void explore(graphint v, graphint mycolor) {
  if (mycolor < 0) {
    auto claimed = d::call(colors+v, [v](graphint * c){
      if (*c < 0) {
        *c = v;
        return true;
      } else {
        return false;
      }
    });
    if (!claimed) return;
    mycolor = v;
  }
  
  // visit neighbors of v
  graphint _c[2]; Incoherent<graphint>::RO c(g.edgeStart+v, 2, _c);
  
  forall_localized_async<GCE>(g.endVertex+c[0], c[1]-c[0], [mycolor](graphint& ev){
    // TODO: make async
    GCE->enroll();
    Core origin = mycore();
    auto colors_ev = colors+ev;
    send_message(colors_ev.core(), [origin,mycolor,colors_ev]{
      auto ec = colors_ev.pointer();
      size_t ev = colors_ev - colors;
      if (*ec < 0) {
        *ec = mycolor;
        publicTask([ev,origin,mycolor]{
          explore<GCE>(ev,mycolor);
          complete(make_global(GCE,origin));
        });
      } else {
        // already had color
        // TODO: avoid spawning task? (i.e. make insert() async)
        privateTask([ec,mycolor,origin]{
          Edge edge = (*ec > mycolor) ? Edge{mycolor,*ec} : Edge{*ec,mycolor};
          component_edges->insert(edge);
          complete(make_global(GCE,origin));
        });
      }
    });
  });
}

/// Takes a graph as input and an array with length NV.  The array D will store
/// the coloring of each component.  The coloring will be using vertex IDs and
/// therefore will be an integer between 0 and NV-1.  The function returns the
/// total number of components.
graphint connectedComponents(graph * in_g) {
  const auto NV = in_g->numVertices;
  
  auto _component_edges = EdgeHashSet::create(FLAGS_cc_hash_size);
  
  auto _colors = global_alloc<graphint>(NV);
  auto _visited = global_alloc<graphint>(NV);
  auto _in_g = *in_g;
  call_on_all_cores([_colors,_visited,_in_g,_component_edges]{
    component_edges = _component_edges;
    colors = _colors;
    visited = _visited;
    g = _in_g;
  });
  
  forall_localized(colors, NV, [](int64_t v, graphint& c){ c = -v-1; });
  
  ///////////////////////////////////////////////////////////////
  // Find component edges
  forall_localized<&gce,256>(colors, NV, [](int64_t v, graphint& c){
    explore<&gce>(v,c);
  });
  
  cc_intermediate_set_size = component_edges->size();
  
  ///////////////////////////////////////////////////////////////
  // 
  int pass = 0;
  do {
    VLOG(0) << "pass " << pass;
    call_on_all_cores([]{ changed = false; });
  
    // Hook
    VLOG(0) << "hook";
    component_edges->forall_keys([](Edge& e){
      graphint ci = d::read(colors+e.start),
               cj = d::read(colors+e.end);
      
      bool lchanged = false;
    
      if ( ci < cj ) {
        lchanged = d::call(colors+cj, [cj](graphint* ccj){
          if (*ccj == cj) { *ccj = cj; return true; }
          else { return false; }
        });
      }
      if (!lchanged && cj < ci) {
        lchanged = d::call(colors+ci, [ci](graphint* cci){
          if (*cci == ci) { *cci = ci; return true; }
          else { return false; }
        });
      }
    
      if (lchanged) { changed = true; }
    });
    
    // Compress
    VLOG(0) << "compress";
    component_edges->forall_keys([](Edge& e){
      auto compress = [](graphint i) {
        graphint ci, cci, nc;
        ci = nc = d::read(colors+i);
        while ( nc != (cci=d::read(colors+nc)) ) { nc = d::read(colors+cci); }
        if (nc != ci) {
          changed = true;
          d::write(colors+i, nc);
        }
      };
      
      compress(e.start);
      compress(e.end);
    });
    pass++;
    call_on_all_cores([]{ VLOG(0) << "changed = " << changed; });
  } while (reduce<bool,collective_or>(&changed) == true);
  
  auto propagate = [](graphint v) {
    
  };
  
  graphint ncomponents = component_edges->size();
  VLOG(0) << "component_edges.size = " << component_edges->size();
  component_edges->forall_keys([](Edge& e){ VLOG(0) << e; });
  
  return ncomponents;
}
