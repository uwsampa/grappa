/////////////////////////////////////
// tutorial/addressing_symmetric.cpp
/////////////////////////////////////
#include <Grappa.hpp>
#include <Collective.hpp>
#include <GlobalAllocator.hpp>

using namespace Grappa;

struct Data {
  size_t N;
  long *buffer;
  
  void init(size_t N) {
    this->N = N;
    this->buffer = new long[32];
  }
} GRAPPA_BLOCK_ALIGNED;

int main(int argc, char *argv[]) {
  init(&argc, &argv);
  run([]{    
    // allocate a copy of Data on every core out of the global heap
    GlobalAddress<Data> d = symmetric_global_alloc< Data >();
    
    on_all_cores([d]{
      // use `->` overload to get pointer to local copy to call the method on
      d->init(1024);
    });
    
    // now we have a local copy of the struct available anywhere
    on_all_cores([d]{
      d->buffer[0] = d->N;
    });
  });
  finalize();
}

