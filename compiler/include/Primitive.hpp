#pragma once

#include <Addressing.hpp>
#include <Communicator.hpp>
#include <Message.hpp>
#include <string.h>

#define global grappa_global

template< typename T >
inline T global* as_ptr(GlobalAddress<T> ga) {
  return reinterpret_cast<T global*>(ga.raw_bits());
}

template< typename T >
inline GlobalAddress<T> as_global_addr(T global* ptr) {
  return GlobalAddress<T>::Raw(reinterpret_cast<intptr_t>(ptr));
}
//template <>
//inline GlobalAddress<char> as_global_addr(void global* ptr) {
//  return GlobalAddress<char>::Raw(reinterpret_cast<intptr_t>(ptr));
//}

namespace Grappa {

  Core core(void global* g) { return as_global_addr(g).core(); }

  template< typename T>
  T* pointer(T global* g) { return as_global_addr(g).pointer(); }
  
  template< typename T >
  inline T global* globalize( T* t, Core n = Grappa::mycore() ) {
    return as_ptr(make_global(t, n));
  }

}

/// most basic way to get data from global address
///
/// TODO: have aggregator implement this directly to save overhead (i.e. instead of serializing/deserializing it can just extract the pointers directly)
extern "C"
void grappa_get(void *addr, void global* ptr, size_t sz) {
  auto ga = as_global_addr(ptr);
  auto origin = Grappa::mycore();
  auto dest = ga.core();
  
  if (dest == origin) {
    
    memcpy(addr, ga.pointer(), sz);
    
  } else {
    
    Grappa::FullEmpty<void*> result(addr); result.reset();
    
    auto g_result = make_global(&result);
    
    Grappa::send_heap_message(dest, [ga,sz,g_result]{
      
      Grappa::send_heap_message(g_result.core(), [g_result](void * payload, size_t psz){
        
        auto r = g_result.pointer();
        auto dest_addr = r->readXX();
        memcpy(dest_addr, payload, psz);
        r->writeEF(dest_addr);
        
      }, ga.pointer(), sz);
      
    });
    
    result.readFF();
  }
}

