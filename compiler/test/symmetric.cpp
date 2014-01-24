#include <Grappa.hpp>
#include <Primitive.hpp>
#include <Collective.hpp>
#include <GlobalAllocator.hpp>

using namespace Grappa;

struct Foo {
  long x, y;
  
  void bar() {
    printf("%ld, %ld\n", x, y);
  }
  
} GRAPPA_BLOCK_ALIGNED;

int main(int argc, char* argv[]) {
  init(&argc, &argv);
  run([]{
    
    Foo symmetric* g = symmetric_global_alloc<Foo>();
    
    on_all_cores([=]{
      
      g->x = mycore();
      g->y = 12345;
      
      g->bar();
      
    });
    
    for (Core c=0; c<cores(); c++) {
      CHECK_EQ( delegate::call(c, [=]{ return symm_addr(g)->x; }), c );
    }
    
  });
  finalize();
}
