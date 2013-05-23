#include "defs.hpp"
#include <Grappa.hpp>
#include <Collective.hpp>
#include <Delegate.hpp>
#include <ParallelLoop.hpp>
#include <GlobalCompletionEvent.hpp>
#include <GlobalHashSet.hpp>
#include <GlobalCounter.hpp>
#include <Array.hpp>

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
static graph g;

static GlobalAddress<graphint> colors;
static GlobalAddress<graphint> visited;
static GlobalAddress<color_t> marks;
static bool changed;
static graphint ncomponents;

static GlobalAddress<GlobalHashSet<Edge>> component_edges;

static GlobalCompletionEvent gce;

///////////////////////////////////////////////////////////////

template< GlobalCompletionEvent * GCE = &impl::local_gce >
void explore(graphint v, graphint mycolor, GlobalAddress<CompletionEvent> owner) {
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
  
  enroll(owner, c[1]-c[0]);
  forall_localized_async<nullptr>(g.endVertex+c[0], c[1]-c[0], [mycolor,owner](graphint& ev){
    // TODO: make async
    Core origin = mycore();
    auto colors_ev = colors+ev;
    send_message(colors_ev.core(), [owner,mycolor,colors_ev]{
      auto ec = colors_ev.pointer();
      size_t ev = colors_ev - colors;
      if (*ec < 0) {
        *ec = mycolor;
        publicTask([ev,mycolor,owner]{
          explore(ev,mycolor,owner);
          complete(owner);
        });
      } else {
        // already had color
        // TODO: avoid spawning task? (i.e. make insert() async)
        Edge edge = (*ec > mycolor) ? Edge{mycolor,*ec} : Edge{*ec,mycolor};
        if (FLAGS_cc_insert_async) {
          component_edges->insert_async(edge, [owner]{ complete(owner); });
        } else {
          privateTask([edge,owner]{
            component_edges->insert(edge);
            complete(owner);
          });
        }
      }
    });
  });
}

template< GlobalCompletionEvent * GCE = &impl::local_gce >
void search(graphint v, graphint mycolor) {
  graphint _c[2]; Incoherent<graphint>::RO c(g.edgeStart+v, 2, _c);
  forall_localized_async<GCE>(g.endVertex+c[0], c[1]-c[0], [mycolor](graphint& ev){
    if (d::fetch_and_add(visited+ev, 1) == 0) {
      d::write(colors+ev, mycolor);
      publicTask<GCE>([ev,mycolor]{ search(ev, mycolor); });
    }
  });
}

void pram_cc() {
  const auto NV = g.numVertices;
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

/// Takes a graph as input and an array with length NV.  The array D will store
/// the coloring of each component.  The coloring will be using vertex IDs and
/// therefore will be an integer between 0 and NV-1.  The function returns the
/// total number of components.
graphint connectedComponents(graph * in_g) {
  double t;
  const auto NV = in_g->numVertices;
  
  auto _component_edges = GlobalHashSet<Edge>::create(FLAGS_cc_hash_size);
  
  auto _colors = global_alloc<graphint>(NV);
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
  
  t = walltime();
  forall_localized(colors, NV, [](int64_t v, graphint& c){ c = -v-1; });
  
  size_t current_root = 0;
  auto root_addr = make_global(&current_root);
  
  // forall_localized<&impl::local_gce,256>(colors, NV, [](int64_t v, graphint& c){
  on_all_cores([root_addr]{
    range_t r = blockDist(0, FLAGS_cc_concurrent_roots, mycore(), cores());
    auto nlocalroots = r.end - r.start;
    
    for (size_t i = 0; i < nlocalroots; i++) {  
      privateTask<&gce>([root_addr]{
        CompletionEvent ce;
        auto cea = make_global(&ce);
        graphint v;
        while ((v=d::fetch_and_add(root_addr,1)) < g.numVertices) {
          explore<nullptr>(v, d::read(colors+v), cea);
          ce.wait();
        }
      });
    }
  });
  gce.wait();
  if (FLAGS_cc_insert_async) { component_edges->sync_all_cores(); }
  
  t = walltime() - t;
  
  LOG(INFO) << "cc_set_size: " << component_edges->size();
  LOG(INFO) << "cc_set_insert_time: " << t;
  if (VLOG_IS_ON(3)) {
    component_edges->forall_keys([](Edge& e){ VLOG(0) << e; });
    VLOG(3) << util::array_str("colors: ", colors, NV, 32);
  }
  
  ///////////////////////////////////////////////////////////////
  // Run classic Connected Components algorithm on reduced graph
  t = walltime();
  pram_cc();
  t = walltime() - t;
  LOG(INFO) << "cc_reduced_graph_time: " << t;
  
  ///////////////////////////////////////////////////////////////
  // Propagate colors out to the rest of the vertices

  t = walltime();
  Grappa::memset(visited, 0, NV);
  component_edges->forall_keys([](Edge& e){
    auto mycolor = d::read(colors+e.start);
    for (auto ev : {e.start, e.end}) {
      publicTask([ev,mycolor]{
        if (d::fetch_and_add(visited+ev, 1) == 0) {
          search(ev,mycolor);
        }
      });
    }
  });
  t = walltime() - t;
  LOG(INFO) << "cc_propagate_time: " << t;
  DVLOG(3) << util::array_str("colors: ", colors, NV, 32);
  
  auto nca = GlobalCounter::create();
  forall_localized(colors, NV, [nca](int64_t v, graphint& c){ if (c == v) nca->incr(); });
  graphint ncomponents = nca->count();
  nca->destroy();
  return ncomponents;
}
