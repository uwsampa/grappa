#include "defs.hpp"
#include <Grappa.hpp>
#include <Collective.hpp>
#include <Delegate.hpp>
#include <ParallelLoop.hpp>
#include <GlobalCompletionEvent.hpp>
#include <GlobalHashSet.hpp>

using namespace Grappa;

DECLARE_int64(cc_hash_size);

struct ColoredVertex {
  union {
    struct {
      bool visited: 1;
      long color: 63;
    };
    intptr_t raw_; // unnecessary; just to ensure alignment
  };
  
  ColoredVertex(): visited(false), color(-1) {}
};

static graph g;

static GlobalAddress<ColoredVertex> colors;
static GlobalAddress<graphint> visited;
static GlobalAddress<color_t> marks;
static graphint nchanged;
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
void explore(graphint v) {
  // delegate probably short-circuited because we try to spawn task at ColoredVertex
  auto mycolor = delegate::call(colors+v, [v](ColoredVertex * c) {
    graphint mycolor;
    if (!c->visited) {
      c->visited = true;
      c->color = mycolor = v;
    } else {
      mycolor = c->color;
    }
    return mycolor;
  });
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
      if (!ec->visited) {
        ec->visited = true;
        ec->color = mycolor;
        publicTask([ev,origin]{
          explore<GCE>(ev);
          complete(make_global(GCE,origin));
        });
      } else {
        // already had color
        // TODO: avoid spawning task? (i.e. make insert() async)
        privateTask([ec,mycolor,origin]{
          Edge edge = (ec->color > mycolor) ? Edge{mycolor,ec->color} : Edge{ec->color,mycolor};
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
  
  auto _colors = global_alloc<ColoredVertex>(NV);
  auto _visited = global_alloc<graphint>(NV);
  auto _in_g = *in_g;
  call_on_all_cores([_colors,_visited,_in_g,_component_edges]{
    component_edges = _component_edges;
    colors = _colors;
    visited = _visited;
    g = _in_g;
  });
  
  ///////////////////////////////////////////////////////////////
  // Find component edges
  forall_localized<&gce,256>(colors, NV, [](int64_t v, ColoredVertex& c){
    explore<&gce>(v);
  });
  
  ///////////////////////////////////////////////////////////////
  // 
  
  graphint ncomponents = component_edges->size();
  VLOG(0) << "component_edges.size = " << component_edges->size();
  component_edges->forall_keys([](Edge& e){ VLOG(0) << e; });
  
  return ncomponents;
}
