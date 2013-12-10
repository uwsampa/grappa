#include <Grappa.hpp>
#include <Primitive.hpp>
#include <Collective.hpp>
#include <GlobalAllocator.hpp>

using namespace Grappa;

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
    Statistics::merge_and_dump_to_file();
  });
  finalize();
  return 0;
}
