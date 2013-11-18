#pragma once

#include <Addressing.hpp>
#include <Communicator.hpp>
#include <Message.hpp>
#include <string.h>
#include <Delegate.hpp>
#include <Array.hpp>

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

extern "C" {
  
  long grappa_read_long(long global* a) {
    return Grappa::delegate::read(gaddr(a));
  }

  long grappa_fetchadd_i64(long global* a, long inc) {
    return Grappa::delegate::fetch_and_add(gaddr(a), inc);
  }

  void* grappa_wide_get_pointer(GlobalAddressBase a) {
    return GlobalAddress<int8_t>(a).pointer();
  }
  Core grappa_wide_get_core(GlobalAddressBase a) {
    return GlobalAddress<int8_t>(a).core();
  }
  GlobalAddressBase grappa_wide_get_locale_core(GlobalAddressBase a, Core* c) {
    *c = GlobalAddress<int8_t>(a).core(); // TODO: get id within locale...
    return a;
  }
  
  GlobalAddressBase grappa_make_wide(Core c, void* p) { return make_global(p, c); }
  
  void global* grappa_wide_to_global_void(GlobalAddressBase ga) {
    return gptr(GlobalAddress<int8_t>(ga));
  }
  GlobalAddressBase grappa_global_to_wide_void(void global* ga) { return gaddr(ga); }
  
  void grappa_memset_void(GlobalAddressBase dst, int8_t val, int64_t n) {
    Grappa::memset(GlobalAddress<int8_t>(dst), val, n);
  }
  
  void grappa_getput_void(GlobalAddressBase dst, GlobalAddressBase src, int64_t n) {
    Grappa::memcpy(GlobalAddress<int8_t>(dst), GlobalAddress<int8_t>(src), n);
  }
  
  /// most basic way to read data from remote address (compiler generates these from global* accesses)
  void grappa_get(void *addr, GlobalAddressBase src, int64_t sz, int64_t sync_control) {
    auto ga = GlobalAddress<int8_t>(src);
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
  void grappa_put(GlobalAddressBase dest, void* src, int64_t sz, int64_t sync_control) {
    auto ga = GlobalAddress<int8_t>(dest);
    auto origin = Grappa::mycore();
    
    if (ga.core() == origin) {
      
      memcpy(ga.pointer(), src, sz);
      
    } else {
      
      Grappa::FullEmpty<bool> result;
      auto g_result = make_global(&result);
      
      Grappa::send_heap_message(ga.core(), [ga,sz,g_result](void * payload, size_t psz){
        
        memcpy(ga.pointer(), payload, sz);
        
        Grappa::send_heap_message(g_result.core(), [g_result]{ g_result->writeEF(true); });
        
      }, src, sz);
      
      result.readFF();
    }
  }

  void grappa_on(Core dst, void (*fn)(void* args, void* out), void* args, size_t args_sz,
                 void* out, size_t out_sz) {
    auto origin = Grappa::mycore();
    
    if (dst == origin) {
      
      fn(args, out);
      
    } else {
      
      Grappa::FullEmpty<void*> fe(out); fe.reset();
      auto gfe = make_global(&fe);
      
      Grappa::send_heap_message(dst, [fn,out_sz,gfe](void* args, size_t args_sz){
        
        char out[out_sz];
        fn(args, out);
        
        Grappa::send_heap_message(gfe.core(), [gfe](void* out, size_t out_sz){
          
          auto r = gfe->readXX();
          memcpy(r, out, out_sz);
          gfe->writeEF(r);
          
        }, out, out_sz);
        
      }, args, args_sz);
      
      fe.readFF();
    }
  }
} // extern "C"