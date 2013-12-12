#include <Grappa.hpp>
#include <Primitive.hpp>
#include <Collective.hpp>
#include <GlobalAllocator.hpp>

using namespace Grappa;

void foo(long& x) {
  long global* xa = make_global(&x);
  long y = 0;
  long global* ya = make_global(&y);
  
  if (*xa < 9) {
    *ya = *xa + 6;
  } else {
    (*ya)++;
    while (*ya <= 1) {
      (*ya)++;
      if (*xa) {
        *xa = 1;
      }
    }
  }
  
  CHECK_EQ(x, 1);
  CHECK_EQ(y, 2);
}

int main(int argc, char* argv[]) {
  init(&argc, &argv);
  run([]{
    
    long x = 0;
    long global* xa = make_global(&x);
    
    on_all_cores([=]{
      
      if (xa != nullptr) {
        long xx = *xa;
        
        if (*xa == 0) {
          *xa = 7;
        }
        
        (*xa)++;
      }
      
    });
    
    VLOG(0) << "x => " << x;
    CHECK_EQ(x, 9);
    
    foo(x);
    
    Statistics::merge_and_dump_to_file();
  });
  finalize();
  return 0;
}
