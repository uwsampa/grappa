#undef __GRAPPA_CLANG__
#warning disabled grappaclang

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

DEFINE_bool( metrics, false, "Dump metrics");

DEFINE_int32(scale, 10, "Log2 number of vertices.");
DEFINE_int32(edgefactor, 16, "Average number of edges per vertex.");
DEFINE_int64(hash_size, 1<<10, "size of GlobalHashSet");
DEFINE_int64(concurrent_roots, 1, "number of concurrent `explores`");
DEFINE_bool(insert_async, false, "do inserts asynchronously");

GRAPPA_DEFINE_METRIC(SimpleMetric<int64_t>, ncomponents, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, components_time, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, graph_create_time, 0);

SymmetricAddress<GlobalHashSet<Edge>> comp_set;
SymmetricAddress<Graph<CCVertex>> g;

const short MAX_CONCURRENT_ROOTS = 1024;
GlobalCompletionEvent c[MAX_CONCURRENT_ROOTS];

const Core ORIGIN = 0;

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
    delegate::call<async,nullptr>(vj, [=](CCVertex& v){
      auto vjj = make_linear(&v) - g->vs;
      CHECK_EQ(j,vjj); // TODO: remove redundant movement
      
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


int main(int argc, char* argv[]) {
  init(&argc, &argv);
  run([]{
    CHECK_LE(FLAGS_concurrent_roots, MAX_CONCURRENT_ROOTS);
    int64_t NE = (1L << FLAGS_scale) * FLAGS_edgefactor;
    bool verified = false;
    double t;
    
    t = walltime();
    
    auto tg = TupleGraph::Kronecker(FLAGS_scale, NE, 111, 222);
    
#ifdef __GRAPPA_CLANG__
    auto _g = as_ptr( Graph<CCVertex>::create( tg ) );
#else
    auto _g = Graph<CCVertex>::create( tg );
#endif
    
    graph_create_time = (walltime()-t);
    LOG(INFO) << graph_create_time;
    
    // initialize
    forall(_g, [](int64_t i, CCVertex& v){ v->init(-i-1); });
    auto _set = GlobalHashSet<Edge>::create(FLAGS_hash_size);
    call_on_all_cores([=]{ comp_set = _set; g = _g; });
    
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
    
    VLOG(0) << "components set: {";
    comp_set->forall_keys([](Edge& e){ VLOG(0) << "  " << e; });
    VLOG(0) << "}";
    
    if (FLAGS_metrics) Metrics::merge_and_print();
    Metrics::merge_and_dump_to_file();
    
  });
  finalize();
}
