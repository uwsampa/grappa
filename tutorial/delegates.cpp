///////////////////////////
// tutorial/delegates.cpp
///////////////////////////
#include <Grappa.hpp>
#include <Delegate.hpp>
#include <Collective.hpp>
#include <GlobalAllocator.hpp>
#include <iostream>

using namespace Grappa;

int main(int argc, char *argv[]) {
  init(&argc, &argv);
  run([]{
    
    size_t N = 50;
    GlobalAddress<long> array = global_alloc<long>(N);
    
    // simple global write
    for (size_t i = 0; i < N; i++) {
      // array[i] = i
      delegate::write( array+i, i );
    }
    
    for (size_t i = 0; i < N; i += 10) {
      // simple remote read
      // value = array[i]
      long value = delegate::read( array+i );
      std::cout << "[" << i << "] = " << value;
      
      // do some arbitrary computation on the core that owns `array+i`
      double v = delegate::call(array+i, [](long *a){ return tan(*a); });
      std::cout << ", tan = " << v << std::endl;
    }
       
  });
  finalize();
}
