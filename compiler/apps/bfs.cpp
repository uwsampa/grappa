#include <Grappa.hpp>
#include <GlobalVector.hpp>
#include <graph/Graph.hpp>
#include <Primitive.hpp>

using namespace Grappa;

DEFINE_int32(scale, 10, "Log2 number of vertices.");
DEFINE_int32(edgefactor, 16, "Average number of edges per vertex.");

GRAPPA_DEFINE_METRIC(SimpleMetric<double>, bfs_mteps, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, bfs_time, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, bfs_nedge, 0);

int64_t *frontier_base, *f_head, *f_tail, *f_level;

GlobalCompletionEvent joiner;

struct BFSVertex : public Vertex {
  struct Data {
    int64_t parent;
    int64_t level;
    bool seen;
  };
  
  BFSVertex() { vertex_data = new Data{ -1, -1, false }; }
  
  Data* operator->() { return reinterpret_cast<Data*>(vertex_data); }
};

int64_t verify(TupleGraph tg, SymmetricAddress<Graph<BFSVertex>> g, int64_t root) {
  delegate::call(g->vs+root, [=](BFSVertex* v){
    v->parent = root;
  });
  CHECK_EQ(  );
}

int main(int argc, char* argv[]) {
  init(&argc, &argv);
  run([]{
    int64_t NE = (1L << FLAGS_scale) * FLAGS_edgefactor;
    
    auto tg = TupleGraph::Kronecker(FLAGS_scale, NE, 111, 222);
    auto g = as_ptr( Graph<BFSVertex>::create( tg ) );
    
    // initialize frontier on each core
    call_on_all_cores([g]{
      frontier_base = f_head = f_tail = f_level = locale_alloc<int64_t>(g->nv / cores() * 2);
    });
    
    // intialize parent to -1
    forall(symm_addr(g), [](BFSVertex& v){ v.parent(-1); });
    
    int64_t root = 0;
    g->vs[root]->parent = 0;
    
    double t = walltime();
    
    on_all_cores([=]{
      joiner.enroll();
      barrier();
      
      auto vs = as_ptr(g->vertices());
      
      int64_t next_level_total;
      
      do {
        f_level = f_tail;
        
        while (f_head < f_level) {
          auto i = *f_head++;  // pop off frontier
          
          for (auto j : vs[i].adj_iter()) {
//            spawn<&joiner>([=]{
              if (vs[j]->parent == -1) {
                vs[j]->parent = i;
                *f_tail++ = j; // push 'j' onto frontier
              }
//            });
          }
        }
        
        joiner.complete();
        joiner.wait();
        
        next_level_total = allreduce<int64_t, collective_add>(f_tail - f_level);
      } while (next_level_total > 0);
      
    });
    
    bfs_time = walltime() - t;
    
    bfs_nedge = verify(tg, g, root);
    
    bfs_mteps = bfs_nedge / bfs_time / 1.0e6;
    
    LOG(INFO) << "\n" << bfs_nedge << "\n" << bfs_time << "\n" << bfs_mteps;
    
    Metrics::merge_and_dump_to_file();
    
  });
  finalize();
}
