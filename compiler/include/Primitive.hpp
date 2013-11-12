#pragma once

#include <Addressing.hpp>
#include <Communicator.hpp>
#include <Message.hpp>
#include <string.h>
#include <Delegate.hpp>

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
long grappa_read_long(long global* a) {
  return Grappa::delegate::read(gaddr(a));
}

/// most basic way to read data from remote address (compiler generates these from global* accesses)
extern "C"
void grappa_get(void *addr, void global* src, size_t sz) {
  auto ga = gaddr(src);
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

/// most basic way to write data at remote address (compiler generates these from global* accesses)
extern "C"
void grappa_put(void global* dest, void* src, size_t sz) {
  auto origin = Grappa::mycore();
  
  if (Grappa::core(dest) == origin) {
    
    memcpy(Grappa::pointer(dest), src, sz);
    
  } else {
    
    Grappa::FullEmpty<bool> result;
    auto g_result = make_global(&result);
    
    Grappa::send_heap_message(Grappa::core(dest), [dest,sz,g_result](void * payload, size_t psz){
      
      memcpy(Grappa::pointer(dest), payload, sz);
      
      Grappa::send_heap_message(g_result.core(), [g_result]{ g_result->writeEF(true); });
      
    }, src, sz);
    
    result.readFF();
  }
}
