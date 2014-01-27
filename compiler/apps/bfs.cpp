#include <Grappa.hpp>
#include <GlobalVector.hpp>
#include <graph/Graph.hpp>
#include <Primitive.hpp>

using namespace Grappa;

DEFINE_int32(scale, 10, "Log2 number of vertices.");
DEFINE_int32(edgefactor, 16, "Average number of edges per vertex.");

int64_t *frontier_base, *f_head, *f_tail, *f_level;

GlobalCompletionEvent joiner;

int main(int argc, char* argv[]) {
  init(&argc, &argv);
  run([]{
    int64_t NE = (1L << FLAGS_scale) * FLAGS_edgefactor;
    
    auto g = as_ptr( Graph<VertexP>::create( TupleGraph::Kronecker(FLAGS_scale, NE, 111, 222) ) );
    
    // initialize frontier on each core
    call_on_all_cores([g]{
      frontier_base = f_head = f_tail = f_level = locale_alloc<int64_t>(g->nv / cores() * 2);
    });
    
    // intialize parent to -1
    forall(symm_addr(g), [](VertexP& v){ v.parent(-1); });
    
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
            spawn<&joiner>([=]{
              if (vs[j].parent() == -1) {
                vs[j].parent( i );
                *f_tail++ = j; // push 'j' onto frontier
              }
            });
          }
        }
        
        joiner.complete();
        joiner.wait();
        
        next_level_total = allreduce<int64_t, collective_add>(f_tail - f_level);
      } while (next_level_total > 0);
      
    });
    
  });
  finalize();
}
