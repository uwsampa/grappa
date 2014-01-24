#include <Grappa.hpp>
#include <Primitive.hpp>
#include <Collective.hpp>
#include <GlobalAllocator.hpp>

using namespace Grappa;

struct Foo {
  long x, y;
  
  void bar(long z) {
    printf("x: %ld, y: %ld, z: %ld\n", x, y, z);
  }
  
} GRAPPA_BLOCK_ALIGNED;

long z;

int main(int argc, char* argv[]) {
  init(&argc, &argv);
  run([]{
    
    Foo symmetric* s = symmetric_global_alloc<Foo>();
    
    on_all_cores([=]{
      
      s->x = mycore();
      s->y = 12345;
      
      s->bar(0);
      
      z = 7 * mycore();
      
    });
    
    for (Core c=0; c<cores(); c++) {
      CHECK_EQ( delegate::call(c, [=]{ return symm_addr(s)->x; }), c );
    }
    
    // test delegates with symmetric addresses
    long global* z1 = make_global(&z, 1);
    
    s->bar(*z1);
    
  });
  finalize();
}
