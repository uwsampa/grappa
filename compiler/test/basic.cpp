#include <Grappa.hpp>
#include <Primitive.hpp>
#include <Collective.hpp>

using namespace Grappa;

int main(int argc, char* argv[]) {
  init(&argc, &argv);
  run([]{
    
    long x = 1;
    long global* xa = ga_make_global(&x);
    
    on_all_cores([xa]{
//      LOG(INFO) << "xa: <" << ga_core(xa) << "," << ga_pointer(xa) << ">";
      long y = *xa;
      LOG(INFO) << "*xa = " << y;
    });
    
  });
  finalize();
  return 0;
}
