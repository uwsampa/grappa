#include <iostream>
#include <cstdint>
#include <string.h>

using namespace std;

#define global __attribute__((address_space(100)))
;

inline short mycore() { return 7; }

template< typename T >
T global* make_global(T* in) {
  return reinterpret_cast<T global*>(reinterpret_cast<intptr_t>(in) + ((size_t)mycore() << 48));
}

template< typename T >
T* extract_pointer(T global* a) {
  return reinterpret_cast<T*>( reinterpret_cast<intptr_t>(a) & ((1L<<48)-1) );
}

extern "C" int delegate_read_int(int global* a) { return 7; }

namespace delegate {
  template< typename T >
  T read(T global* a) { return *a; }
}

extern "C"
void grappa_get(void* addr, void global* g_addr, size_t sz) {
  fprintf(stderr, "grappa_get(%p, %p, %ld)\n", addr, reinterpret_cast<void*>(g_addr), sz);
  memcpy(addr, extract_pointer(g_addr), sz);
}

int main(int argc, char* argv[]) {
  int x = 1;
  int global* a = make_global(&x);
  // a = &x;
  
  std::cout << "a: [" << reinterpret_cast<void*>(a) << "] = " << *a << "\n";
  
  return 0;
}
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
//> test.cpp:18] a: [1] = 
//<pid 10848 SIGSEGV (signal 11)>
/////////////////////////////////////////////////////////////
