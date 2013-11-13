#include <Grappa.hpp>
#include <Primitive.hpp>
#include <Collective.hpp>
#include <GlobalAllocator.hpp>

using namespace Grappa;

int main(int argc, char* argv[]) {
  init(&argc, &argv);
  run([]{
    
    long x = 1, y = 7;
    long global* xa = make_global(&x);
    long global* ya = make_global(&y);
    
    long global* array = global_alloc<long>(10);
    
    on_all_cores([=]{
      LOG(INFO) << gaddr(xa);
      
      long y = *xa;
      long z = *ya;
      long w = *xa;
      
      LOG(INFO) << "*xa = " << y;
      CHECK(z == 7);
      CHECK(y == w);
      
      long i = (*xa)++;
      CHECK(i >= 1);
      
      if (mycore() == 0) {
        for (long i=0; i<10; i++) {
          array[i] = i;
        }
        barrier();
      } else {
        barrier();
        long total = 0;
        for (long i=0; i<10; i++) {
          total += array[i];
        }
        LOG(INFO) << "total: " << total;
      }
      
    });
    
  });
  finalize();
  return 0;
}
