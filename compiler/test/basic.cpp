#include <Grappa.hpp>
#include <Primitive.hpp>
#include <Collective.hpp>
#include <GlobalAllocator.hpp>

using namespace Grappa;

int main(int argc, char* argv[]) {
  init(&argc, &argv);
  run([]{
    
    long x = 1;
    long global* xa = make_global(&x);
    
    long global* array = global_alloc<long>(10);
    
    on_all_cores([xa,array]{
      LOG(INFO) << as_global_addr(xa);
      long y = *xa;
      LOG(INFO) << "*xa = " << y;
      
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
