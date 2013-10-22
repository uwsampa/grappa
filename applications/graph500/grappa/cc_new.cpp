#include "graph.hpp"
#include <GlobalHashSet.hpp>
using namespace Grappa;
namespace d = Grappa::delegate;

DEFINE_int64(cc_hash_size, 1<<10, "size of GlobalHashSet");
DEFINE_int64(cc_concurrent_roots, 1, "number of concurrent `explores` to have at a given time");
DEFINE_bool(cc_insert_async, false, "do inserts asynchronously (keeps them local until end)");

#define GCE (&impl::local_gce)

struct Edge {
  long start, end;
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

class VertexCC : public Vertex {
  struct Data {
    long color   : 63;
    bool visited :  1;
  };
  Data& data() { return *reinterpret_cast<Data*>(&vertex_data); }
public:
  void init(long c = -1, bool v = false) {
    color(c);
    visited(v);
  }

  long color() { return data().color; }
  void color(long c) { data().color = c; }
  long visited() { return data().visited; }
  void visited(long c) { data().visited = c; }
};

long color(GlobalAddress<VertexCC> v) {
  return delegate::call(v, [](VertexCC* v){ return v->color(); });
}
void color(GlobalAddress<VertexCC> v, long c) {
  return delegate::call(v, [c](VertexCC* v){ v->color(c); });
}

///////////////////////////////////////////////////////////////
// Globals
static GlobalAddress<Graph<VertexCC>> g;
static GlobalAddress<GlobalHashSet<Edge>> component_edges;

static bool changed;
static long ncomponents;

void pram_cc() {
  int npass = 0;
  do {
    DVLOG(2) << "npass " << npass;
    call_on_all_cores([]{ changed = false; });
    
    // Hook
    DVLOG(2) << "hook";
    component_edges->forall_keys([](Edge& e){
      long i = e.start, j = e.end;
      CHECK_LT(i, g->nv);
      CHECK_LT(j, g->nv);
      long ci = color(g->vs+e.start),
               cj = color(g->vs+e.end);
      CHECK_LT(ci, g->nv);
      CHECK_LT(cj, g->nv);
      bool lchanged = false;
      
      if ( ci < cj ) {
        lchanged |= d::call(g->vs+cj, [ci,cj](VertexCC* v){
          if (v->color() == cj) { v->color(ci); return true; }
          else { return false; }
        });
      }
      if (!lchanged && cj < ci) {
        lchanged |= d::call(g->vs+ci, [ci,cj](VertexCC* v){
          if (v->color() == ci) { v->color(cj); return true; }
          else { return false; }
        });
      }
      
      if (lchanged) { changed = true; }
    });
    
    // Compress
    DVLOG(2) << "compress";
    component_edges->forall_keys([](Edge& e){
      auto compress = [](long i) {
        long ci, cci, nc;
        ci = nc = color(g->vs+i);
        CHECK_LT(ci, g->nv);
        CHECK_LT(nc, g->nv);
        while ( nc != (cci=color(g->vs+nc)) ) { nc = color(g->vs+cci); CHECK_LT(nc, g->nv); }
        if (nc != ci) {
          changed = true;
          color(g->vs+i, nc);
        }
      };
      
      compress(e.start);
      compress(e.end);
    });
    npass++;
  } while (reduce<bool,collective_or>(&changed) == true);
  
  LOG(INFO) << "cc_pram_passes: " << npass;
}


void explore(long root_index, long mycolor, GlobalAddress<CompletionEvent> ce) {
  auto root_addr = g->vs+root_index;
  CHECK_EQ(root_addr.core(), mycore());
  auto& rv = *root_addr.pointer();
  
  if (mycolor < 0) {
    if (rv.color() < 0) {
      rv.color(root_index);
      mycolor = root_index;
    } else {
      goto explore_done;
    }
  }
  
  // now visit adjacencies
  enroll(ce, rv.nadj);
  for (auto& ev : rv.adj_iter()) {
    send_heap_message((g->vs+ev).core(), [ev,mycolor,ce]{
      auto& v = *(g->vs+ev).pointer();
      
      if (v.color() < 0) { // unclaimed
        v.color(mycolor);
        privateTask([ev,mycolor,ce]{
          explore(ev, mycolor, ce);
        });
      } else { // already had color
        Edge edge{ std::min(v.color(),mycolor), std::max(v.color(),mycolor) };
        if (FLAGS_cc_insert_async) {
          component_edges->insert_async(edge, [ce]{ complete(ce); });
        } else {
          privateTask([edge,ce]{
            component_edges->insert(edge);
            complete(ce);
          });
        }
      }
    });
  }
  
explore_done:
  complete(ce);
}

void search(long v, long mycolor) {
  CHECK_EQ((g->vs+v).core(), mycore());
  auto& src_v = *(g->vs+v).pointer();
  
  GCE->enroll(src_v.nadj);
  for (auto& ev : src_v.adj_iter()) {
    Core origin = mycore();
    send_heap_message((g->vs+ev).core(), [ev,mycolor,origin]{
      auto& v = *(g->vs+ev).pointer();
      if (!v.visited()) {
        v.visited(true);
        v.color(mycolor);
        privateTask([ev,mycolor,origin]{
          search(ev, mycolor);
          GCE->send_completion(origin);
        });
      } else {
        GCE->send_completion(origin);
      }
    });
  }
}

long cc_benchmark(GlobalAddress<Graph<>> in) {
  Statistics::start_tracing();
  
  VLOG(0) << "cc_version: new";
  double t = walltime();
  
  auto _g = Graph<>::transform_vertices<VertexCC>(in, [](long i, VertexCC& v){
    v.init(-i-1);
  });
  auto _component_edges = GlobalHashSet<Edge>::create(FLAGS_cc_hash_size);
  
  call_on_all_cores([_g,_component_edges]{
    g = _g;
    component_edges = _component_edges;
  });
  
  GRAPPA_TIME_LOG("cc_set_insert_time") {
    
    CountingSemaphore _sem(FLAGS_cc_concurrent_roots);
    auto sem = make_global(&_sem);
    
    for (size_t i = 0; i < g->nv; i++) {
      sem->decrement(); // (after filling, blocks until an exploration finishes)
      send_heap_message((g->vs+i).core(), [i,sem]{
        privateTask([i,sem]{
          CompletionEvent ce(1);
          explore(i, (g->vs+i)->color(), make_global(&ce));
          ce.wait();
          send_heap_message(sem.core(), [sem]{ sem->increment(); });
        });
      });
    }
    if (FLAGS_cc_insert_async) component_edges->sync_all_cores();
  }
  
  if (VLOG_IS_ON(3)) {
    VLOG(0) << "components set: {";
    component_edges->forall_keys([](Edge& e){ VLOG(0) << "  " << e; });
    VLOG(0) << "}";
  }
  
  ///////////////////////////////////////////////////////////////
  // Run classic Connected Components algorithm on reduced graph
  GRAPPA_TIME_LOG("cc_reduced_graph_time") {
    pram_cc();
  }
  
  ///////////////////////////////////////////////////////////////
  // Propagate colors out to the rest of the vertices
  GRAPPA_TIME_LOG("cc_propagate_time") {
    // reset 'visited' flag
    forall_localized(g->vs, g->nv, [](VertexCC& v){ v.visited(false); });
    
    component_edges->forall_keys<GCE>([](Edge& e){
      auto mycolor = color(g->vs+e.start);
      for (auto ev : {e.start, e.end}) {
        GCE->enroll();
        Core origin = mycore();
        send_message((g->vs+ev).core(), [ev,mycolor,origin]{
          auto& v = *(g->vs+ev).pointer();
          if (!v.visited()) {
            v.visited(true);
            privateTask([ev,mycolor,origin]{
              search(ev, mycolor);
              GCE->send_completion(origin);
            });
          } else {
            GCE->send_completion(origin);
          }
        });
      }
    });
    
  } // cc_propagate_time

  forall_localized(g->vs, g->nv, [](int64_t i, VertexCC& v){ if (v.color() == i) ncomponents++; });
  long nc = reduce<long,collective_add>(&ncomponents);
  
  t = walltime() - t;
  
  Statistics::stop_tracing();
  
  LOG(INFO) << "ncomponents: " << nc << std::endl;
  LOG(INFO) << "components_time: " << t << std::endl;
  LOG(INFO) << "set_size: " << component_edges->size();
  
  Statistics::merge_and_print();
  
  return nc;
}