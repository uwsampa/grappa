#pragma once

#include <Addressing.hpp>
#include <Communicator.hpp>

#ifndef grappa_global
#error Must use grappaclang with this header (__GRAPPA_CLANG__ not defined)
#endif

#define global grappa_global


template< typename T >
inline T global* gptr(GlobalAddress<T> ga) {
  return reinterpret_cast<T global*>(ga.raw_bits());
}

template< typename T >
inline GlobalAddress<T> gaddr(T global* ptr) {
  return GlobalAddress<T>::Raw(reinterpret_cast<intptr_t>(ptr));
}
//template <>
//inline GlobalAddress<char> gaddr(void global* ptr) {
//  return GlobalAddress<char>::Raw(reinterpret_cast<intptr_t>(ptr));
//}

namespace Grappa {

  Core core(void global* g) { return gaddr(g).core(); }

  template< typename T>
  T* pointer(T global* g) { return gaddr(g).pointer(); }
  
  template< typename T >
  inline T global* globalize( T* t, Core n = Grappa::mycore() ) {
    return gptr(make_global(t, n));
  }
  
}

extern "C"
Core grappa_get_core(void global* g) { return Grappa::core(g); }

extern "C"
void* grappa_get_pointer(void global* g) { return Grappa::pointer(g); }

//extern "C"
//long grappa_read_long(long global* a) {
//  return Grappa::delegate::read(gaddr(a));
//}
//
//extern "C"
//long grappa_fetchadd_i64(long global* a, long inc) {
//  return Grappa::delegate::fetch_and_add(gaddr(a), inc);
//}

extern "C"
void myprint_i64(long v) {
  printf("dbg: %ld\n", v);
}


/// most basic way to read data from remote address (compiler generates these from global* accesses)
extern "C"
void grappa_get(void *addr, void global* src, size_t sz);

/// most basic way to write data at remote address (compiler generates these from global* accesses)
extern "C"
void grappa_put(void global* dest, void* src, size_t sz);

extern "C"
void grappa_on(Core dst, void (*fn)(void* args, void* out), void* args, size_t args_sz,
               void* out, size_t out_sz);

