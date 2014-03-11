#include <Grappa.hpp>
#include <GlobalHashSet.hpp>
#include <graph/Graph.hpp>

#ifdef __GRAPPA_CLANG__
#include <Primitive.hpp>
#endif

using namespace Grappa;

using color_t = long;

struct Edge {
  int64_t start, end;
  Edge(int64_t start, int64_t end): start(start), end(end) {}
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


struct CCData {
  color_t color;
  bool visited;
  
  void init(color_t c = -1, bool v = false) {
    color = c;
    visited = v;
  }
};

using CCVertex = Vertex<CCData>;

color_t color(GlobalAddress<CCVertex> v) {
  return delegate::call(v, [](CCVertex& v){ return v->color; });
}
void color(GlobalAddress<CCVertex> v, color_t c) {
  return delegate::call(v, [c](CCVertex& v){ v->color = c; });
}

#ifdef __GRAPPA_CLANG__
color_t color(CCVertex global* v) { return color(as_addr(v)); }
void color(CCVertex global* v, color_t c) { color(as_addr(v), c); }
#endif

DEFINE_bool( metrics, false, "Dump metrics");

DEFINE_int32(scale, 10, "Log2 number of vertices.");
DEFINE_int32(edgefactor, 16, "Average number of edges per vertex.");
DEFINE_int64(hash_size, 1<<10, "size of GlobalHashSet");
DEFINE_int64(concurrent_roots, 1, "number of concurrent `explores`");
DEFINE_bool(insert_async, false, "do inserts asynchronously");

GRAPPA_DEFINE_METRIC(SimpleMetric<int64_t>, ncomponents, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<int64_t>, pram_passes, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<int64_t>, set_size, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, set_insert_time, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, pram_time, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, propagate_time, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, components_time, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, graph_create_time, 0);

#ifdef __GRAPPA_CLANG__
GlobalHashSet<Edge> symmetric* comp_set;
Graph<CCVertex> symmetric* g;
#else
SymmetricAddress<GlobalHashSet<Edge>> comp_set;
SymmetricAddress<Graph<CCVertex>> g;
#endif

GlobalCompletionEvent c;
bool changed;

int64_t nc;

using delegate::call;

void pram_cc() {
  int npass = 0;
  do {
    DVLOG(2) << "npass " << npass;
    call_on_all_cores([]{ changed = false; });
    
    // Hook
    DVLOG(2) << "hook";
    comp_set->forall_keys([](Edge& e){
#ifdef __GRAPPA_CLANG__
      auto ga = as_addr(g);
#else
      auto ga = g;
#endif
      
      long i = e.start, j = e.end;
      CHECK_LT(i, ga->nv);
      CHECK_LT(j, ga->nv);
      long ci = color(ga->vs+e.start),
               cj = color(ga->vs+e.end);
      CHECK_LT(ci, ga->nv);
      CHECK_LT(cj, ga->nv);
      bool lchanged = false;
      
      if ( ci < cj ) {
        lchanged |= call(ga->vs+cj, [ci,cj](CCVertex& v){
          if (v->color == cj) { v->color = ci; return true; }
          else { return false; }
        });
      }
      if (!lchanged && cj < ci) {
        lchanged |= call(ga->vs+ci, [ci,cj](CCVertex& v){
          if (v->color == ci) { v->color = cj; return true; }
          else { return false; }
        });
      }
      
      if (lchanged) { changed = true; }
    });
    
    // Compress
    DVLOG(2) << "compress";
    comp_set->forall_keys([](Edge& e){
      auto ga = as_addr(g);

      auto compress = [ga](long i) {
        long ci, cci, nc;
        ci = nc = color(ga->vs+i);
        CHECK_LT(ci, ga->nv);
        CHECK_LT(nc, ga->nv);
        while ( nc != (cci=color(ga->vs+nc)) ) { nc = color(ga->vs+cci); CHECK_LT(nc, ga->nv); }
        if (nc != ci) {
          changed = true;
          color(ga->vs+i, nc);
        }
      };
      
      compress(e.start);
      compress(e.end);
    });
    npass++;
  } while (reduce<bool,collective_or>(&changed) == true);
  
  pram_passes = npass;
}

#ifdef __GRAPPA_CLANG__
void explore(int64_t r, color_t mycolor, CompletionEvent global* ce) {
  auto vs = as_ptr(g->vertices());
  auto& v = vs[r];
  
  if (mycolor < 0) {
    if (v->color < 0) {
      v->color = r;
      mycolor = r;
    } else {
      complete(ce);
      return;
    }
  }
  
  // now visit adjacencies
  enroll(ce, v.nadj);
  
  forall<async,nullptr>(adj(g,vs+r), [=](int64_t j){
    auto& vj = vs[j];
    if (vj->color < 0) {
      vj->color = mycolor;
      // mark 'spawn' as 'anywhere'
      spawn([=]{
        explore(j, mycolor, ce);
      });
    } else if (vj->color != mycolor) {
      Edge edge(mycolor, vj->color);
      if (vj->color < mycolor) {
        edge.start = vj->color;
        edge.end = mycolor;
      }
      comp_set->insert_async(edge, [=]{ complete(ce); });
      // TODO: mark as 'anywhere', don't inline those, just assume they're okay
    } else {
      complete(ce);
    }
  });
  
  complete(ce);

}
#else
void explore(int64_t root_index, color_t mycolor, GlobalAddress<CompletionEvent> ce) {
  auto root_addr = g->vs+root_index;
  CHECK_EQ(root_addr.core(), mycore());
  auto& rv = *root_addr.pointer();
  
  if (mycolor < 0) {
    if (rv->color < 0) {
      rv->color = root_index;
      mycolor = root_index;
    } else {
      complete(ce);
      return;
    }
  }
  
  // now visit adjacencies
  enroll(ce, rv.nadj);
  
  forall<async,nullptr>(adj(g,rv), [=](int64_t j, GlobalAddress<CCVertex> vj){
    delegate::call<async,nullptr>(vj, [mycolor,ce](CCVertex& v){
      auto j = make_linear(&v) - g->vs;
      
      if (v->color < 0) { // unclaimed
        v->color = mycolor;
        spawn([=]{
          explore(j, mycolor, ce);
        });
      } else if (v->color != mycolor) {
        Edge edge{ std::min(v->color,mycolor), std::max(v->color,mycolor) };
        comp_set->insert_async(edge, [=]{ complete(ce); });
      } else {
        complete(ce);
      }
    });
  });
  
  complete(ce);
}
#endif

void propagate(int64_t v, color_t mycolor) {

#ifdef __GRAPPA_CLANG__
  auto ga = as_addr(g);
#else
  auto ga = g;
#endif

  CHECK(v < ga->nv && v >= 0) << "-- v: " << v << ", nv: " << ga->nv;
  
  CHECK_EQ((ga->vs+v).core(), mycore());
  auto& rv = *(ga->vs+v).pointer();
  
  c.enroll(rv.nadj);
  forall<async,nullptr>(adj(ga,v), [=](int64_t j, GlobalAddress<CCVertex> vj){
    CHECK(j < ga->nv && j >= 0) << "-- j: " << j << ", vj: " << vj << "\nvs: " << ga->vertices();
    Core origin = mycore();
    call<async,nullptr>(vj, [=](CCVertex& v){      
      if (!v->visited) {
        v->visited = true;
        v->color = mycolor;
        spawn([=]{
          propagate(j, mycolor);
          c.send_completion(origin);
        });
      } else {
        c.send_completion(origin);
      }
    });
  });
}

int main(int argc, char* argv[]) {
  init(&argc, &argv);
  run([]{
    int64_t NE = (1L << FLAGS_scale) * FLAGS_edgefactor;
    double t;
    
    t = walltime();
    
    auto tg = TupleGraph::Kronecker(FLAGS_scale, NE, 111, 222);
    
    auto _g = Graph<CCVertex>::create( tg );
    
    graph_create_time = (walltime()-t);
    LOG(INFO) << graph_create_time;
        
    // initialize
    auto _set = GlobalHashSet<Edge>::create(FLAGS_hash_size);
    
    t = walltime();
    
    forall(_g, [](int64_t i, CCVertex& v){ v->init(-i-1); });
    call_on_all_cores([=]{
#ifdef __GRAPPA_CLANG__
      comp_set = as_ptr(_set);
      g = as_ptr(_g);
#else
      comp_set = _set;
      g = _g;
#endif
    });
    
    
    GRAPPA_TIME_REGION(set_insert_time) {
      // create component edge set (do a bunch of traversals)
      CountingSemaphore _sem(FLAGS_concurrent_roots);
      auto sem = make_global(&_sem);
      
      for (size_t i = 0; i < g->nv; i++) {
        sem->decrement(); // (after filling, blocks until an exploration finishes)
        delegate::call<async,nullptr>(g->vs+i, [=](CCVertex& v){
          auto mycolor = v->color;
          spawn([=]{
            CompletionEvent ce(1);
            explore(i, mycolor, make_global(&ce));
            ce.wait();
            send_heap_message(sem.core(), [sem]{ sem->increment(); });
          });
        });
      }
      comp_set->sync_all_cores();
    }
    LOG(INFO) << set_insert_time;
    set_size = comp_set->size();
    
    if (VLOG_IS_ON(1)) {
      VLOG(0) << "components set: {";
      comp_set->forall_keys([](Edge& e){ VLOG(0) << "  " << e; });
      VLOG(0) << "}";
    }
    
    GRAPPA_TIME_REGION(pram_time) {
      pram_cc();
    }
    LOG(INFO) << pram_time;
    
    ///////////////////////////////////////////////////////////////
    // Propagate colors out to the rest of the vertices
    GRAPPA_TIME_REGION(propagate_time) {
      // reset 'visited' flag
      forall(g, [](CCVertex& v){ v->visited = false; });
    
      comp_set->forall_keys<&c>([](Edge& e){
        auto mycolor = color(g->vs+e.start);
        for (auto ev : {e.start, e.end}) {
          c.enroll();
          Core origin = mycore();
          call<async,nullptr>(g->vs+ev, [=](CCVertex& v){
            if (!v->visited) {
              v->visited = true;
              spawn([ev,mycolor,origin]{
                propagate(ev, mycolor);
                c.send_completion(origin);
              });
            } else {
              c.send_completion(origin);
            }
          });
        }
      });
    
    } // cc_propagate_time
    LOG(INFO) << propagate_time;
    
    forall(g, [](int64_t i, CCVertex& v){ if (v->color == i) nc++; });
    
    ncomponents = reduce<int64_t,collective_add>(&nc);
    components_time = (walltime()-t);
    
    if (FLAGS_metrics) Metrics::merge_and_print();
    else {
      LOG(INFO) << "\n" << set_insert_time
                << "\n" << pram_time
                << "\n" << propagate_time
                << "\n" << ncomponents
                << "\n" << set_size
                << "\n" << components_time;
    }
    Metrics::merge_and_dump_to_file();
    
  });
  finalize();
}
