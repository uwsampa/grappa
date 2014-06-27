#include <Grappa.hpp>
#include <GlobalHashSet.hpp>
#include <graph/Graph.hpp>

using namespace Grappa;
namespace d = Grappa::delegate;

DECLARE_int64(hash_size);
DECLARE_int64(concurrent_roots);

GRAPPA_DECLARE_METRIC(SimpleMetric<int64_t>, pram_passes);
GRAPPA_DECLARE_METRIC(SimpleMetric<int64_t>, set_size);
GRAPPA_DECLARE_METRIC(SimpleMetric<double>, set_insert_time);
GRAPPA_DECLARE_METRIC(SimpleMetric<double>, pram_time);
GRAPPA_DECLARE_METRIC(SimpleMetric<double>, propagate_time);
GRAPPA_DECLARE_METRIC(SimpleMetric<double>, components_time);
GRAPPA_DECLARE_METRIC(SimpleMetric<double>, graph_create_time);


extern GlobalCompletionEvent phaser;

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
      size_t seed = e.end;
      return e.start + 0x9e3779b9 + (seed<<6) + (seed>>2);
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

using G = Graph<CCData,Empty>;


color_t color(GlobalAddress<G::Vertex> v) {
  return delegate::call(v, [](G::Vertex& v){ return v->color; });
}
void color(GlobalAddress<G::Vertex> v, color_t c) {
  return delegate::call(v, [c](G::Vertex& v){ v->color = c; });
}


GlobalAddress<GlobalHashSet<Edge>> comp_set;
GlobalAddress<G> g;

std::unordered_set<Edge> local_set;

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
      long i = e.start, j = e.end;
      CHECK_LT(i, g->nv);
      CHECK_LT(j, g->nv);
      long ci = color(g->vs+e.start),
               cj = color(g->vs+e.end);
      CHECK_LT(ci, g->nv);
      CHECK_LT(cj, g->nv);
      bool lchanged = false;
      
      if ( ci < cj ) {
        lchanged |= call(g->vs+cj, [ci,cj](G::Vertex& v){
          if (v->color == cj) { v->color = ci; return true; }
          else { return false; }
        });
      }
      if (!lchanged && cj < ci) {
        lchanged |= call(g->vs+ci, [ci,cj](G::Vertex& v){
          if (v->color == ci) { v->color = cj; return true; }
          else { return false; }
        });
      }
      
      if (lchanged) { changed = true; }
    });
    
    // Compress
    DVLOG(2) << "compress";
    comp_set->forall_keys([](Edge& e){

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
  
  pram_passes = npass;
}

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
  
  forall<async,nullptr>(adj(g,rv), [=](G::Edge& e){
    delegate::call<async,nullptr>(e.ga, [mycolor,ce](G::Vertex& v){
      auto j = make_linear(&v) - g->vs;
      
      if (v->color < 0) { // unclaimed
        v->color = mycolor;
        spawn([=]{
          explore(j, mycolor, ce);
        });
      } else if (v->color != mycolor) {
        Edge edge{ std::min(v->color,mycolor), std::max(v->color,mycolor) };
        local_set.insert(edge);
        complete(ce);
      } else {
        complete(ce);
      }
    });
  });
  
  complete(ce);
}

void propagate(int64_t v, color_t mycolor) {
  
  CHECK(v < g->nv && v >= 0) << "-- v: " << v << ", nv: " << g->nv;
  
  CHECK_EQ((g->vs+v).core(), mycore());
  auto& rv = *(g->vs+v).pointer();
  
  phaser.enroll(rv.nadj);
  forall<async,nullptr>(adj(g,v), [=](G::Edge& e){
    CHECK(e.id < g->nv && e.id >= 0) << "-- j: " << e.id << ", vj: " << e.ga << "\nvs: " << g->vs;
    Core origin = mycore();
    call<async,nullptr>(e.ga, [=](G::Vertex& v){
      auto j = make_linear(&v) - g->vs;
      if (!v->visited) {
        v->visited = true;
        v->color = mycolor;
        spawn([=]{
          propagate(j, mycolor);
          phaser.send_completion(origin);
        });
      } else {
        phaser.send_completion(origin);
      }
    });
  });
}

size_t connected_components(GlobalAddress<G> _g) {
  // initialize
  auto _set = GlobalHashSet<Edge>::create(FLAGS_hash_size);
  
  double t = walltime();
  
  forall(_g, [](int64_t i, G::Vertex& v){ v->init(-i-1); });
  call_on_all_cores([=]{
    comp_set = _set;
    g = _g;
  });
    
  GRAPPA_TIME_REGION(set_insert_time) {
    // create component edge set (do a bunch of traversals)
    CountingSemaphore _sem(FLAGS_concurrent_roots);
    auto sem = make_global(&_sem);
    
    for (size_t i = 0; i < g->nv; i++) {
      sem->decrement(); // (after filling, blocks until an exploration finishes)
      delegate::call<async,nullptr>(g->vs+i, [=](G::Vertex& v){
        auto signal = [sem]{
          send_heap_message(sem.core(), [sem]{ sem->increment(); });
        };
        auto mycolor = v->color;
        
        if (v.nadj == 0) {
          v->color = mycolor;
          signal();
        } else {
          spawn([=]{
            CompletionEvent ce(1);
            explore(i, mycolor, make_global(&ce));
            ce.wait();
            signal();
          });
        }
      });
    }
    sem->decrement(); // (after filling, blocks until an exploration finishes)
  }
  
  LOG(INFO) << set_insert_time;
    
  GRAPPA_TIME_LOG("fill_global_set_time") {
    // auto ct = sum_all_cores([]{ return local_set.size(); });
    // VLOG(0) << "total set size: " << ct;
    // phaser.enroll(ct);
    on_all_cores([=]{
      for (const Edge& e : local_set) {
        // comp_set->insert_async(e, []{ phaser.send_completion(0); });
        comp_set->insert(e);
      }
    });
    // comp_set->sync_all_cores();
    // phaser.wait();
  }
  
  set_size = comp_set->size();
  LOG(INFO) << set_size;
  
  if (VLOG_IS_ON(2)) {
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
    forall(g, [](G::Vertex& v){ v->visited = false; });
  
    comp_set->forall_keys<&phaser>([](Edge& e){
      auto mycolor = color(g->vs+e.start);
      for (auto ev : {e.start, e.end}) {
        phaser.enroll();
        Core origin = mycore();
        call<async,nullptr>(g->vs+ev, [=](G::Vertex& v){
          if (!v->visited) {
            v->visited = true;
            spawn([ev,mycolor,origin]{
              propagate(ev, mycolor);
              phaser.send_completion(origin);
            });
          } else {
            phaser.send_completion(origin);
          }
        });
      }
    });
  
  } // cc_propagate_time
  components_time = (walltime()-t);
  LOG(INFO) << propagate_time;
  
  forall(g, [](int64_t i, G::Vertex& v){ if (v->color == i) nc++; });
  
  auto ncomponents = reduce<int64_t,collective_add>(&nc);
  return ncomponents;
}
