//////////////////////////////////
// tutorial/addressing_linear.cpp
//////////////////////////////////
#include <Grappa.hpp>
#include <GlobalAllocator.hpp>

using namespace Grappa;

int main(int argc, char *argv[]) {
  init(&argc, &argv);
  run([]{
    auto array = global_alloc<long>(48);
    for (auto i=0; i<48; i++) {
      std::cout << "[" << i << ": core " << (array+i).core() << "] ";
    }
    std::cout << "\n";
  });
  finalize();
}

//> srun --nodes=2 --ntasks-per-node=2 -- tutorial/addressing_linear.exe
