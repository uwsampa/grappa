#pragma once

#include <Addressing.hpp>
#include <Communicator.hpp>
#include <Message.hpp>
#include <string.h>
#include <Delegate.hpp>

#define global grappa_global
#define symmetric grappa_symmetric
#define async_fn grappa_async_fn
#define anywhere grappa_anywhere

#define callable_anywhere __attribute__((annotate("unbound")))


template< typename T >
inline T global* gptr(GlobalAddress<T> ga) {
  return reinterpret_cast<T global*>(ga.raw_bits());
}

template< typename T >
inline GlobalAddress<T> gaddr(T global* ptr) {
  return GlobalAddress<T>::Raw(reinterpret_cast<intptr_t>(ptr));
}

template< typename T >
inline SymmetricAddress<T> symm_addr(T symmetric* ptr) {
  return SymmetricAddress<T>(ptr);
}


namespace Grappa {

  Core core(void global* g) { return gaddr(g).core(); }

  template< typename T>
  T* pointer(T global* g) { return gaddr(g).pointer(); }

  template< typename T>
  T* pointer(T symmetric* g) { return symm_addr(g).pointer(); }
  
  template< typename T >
  inline T global* globalize( T* t, Core n = Grappa::mycore() ) {
    return gptr(make_global(t, n));
  }
  
  /// Helper to make it simple to get 'T global*' from a GlobalAddress<T>
  template< typename T >
  inline T global* as_ptr(GlobalAddress<T> a) { return a; }
  
  /// Helper to make it simple to get 'T symmetric*' from a SymmetricAddress<T>
  template< typename T >
  inline T symmetric* as_ptr(SymmetricAddress<T> a) { return a; }
  
}

extern "C"
Core grappa_get_core(void global* g) { return Grappa::core(g); }

extern "C"
void* grappa_get_pointer(void global* g) { return Grappa::pointer(g); }

extern "C"
void* grappa_get_pointer_symmetric(void symmetric* g) {
  return SymmetricAddress<int8_t>(reinterpret_cast<int8_t symmetric*>(g)).pointer();
}

extern "C"
void global* grappa_make_gptr(void * p, Core c) {
  return make_global(p, c);
}

extern "C"
long grappa_read_long(long global* a) {
  return Grappa::delegate::read(gaddr(a));
}

extern "C"
int64_t grappa_get_i64(int64_t global* a) {
  return Grappa::delegate::read(gaddr(a));
}

extern "C"
int64_t grappa_get_i64_on(Core c, int64_t * a) {
  return Grappa::delegate::call(c, [a]{ return *a; });
}

extern "C"
void grappa_put_i64(int64_t global* a, int64_t v) {
  Grappa::delegate::write(gaddr(a), v);
}

extern "C"
void grappa_put_i64_on(Core c, int64_t * a, int64_t v) {
  Grappa::delegate::call(c, [a,v]{ *a = v; });
}

extern "C"
long grappa_fetchadd_i64(long global* a, long inc) {
  return Grappa::delegate::fetch_and_add(gaddr(a), inc);
}

extern "C"
void myprint_i64(long v) {
  printf("dbg: %ld\n", v);
}


namespace Grappa {
  namespace impl {
    template< bool GPTR >
    void* resolve_ptr(void *ptr) {
      return GPTR ? gaddr(reinterpret_cast<void global*>(ptr)).pointer()
                  : ptr;
    }
    
    template< bool GPTR >
    void do_get(void *addr, Core src_core, void *src_ptr, size_t sz) {
      auto origin = Grappa::mycore();
      
      if (src_core == origin) {
        
        memcpy(addr, resolve_ptr<GPTR>(src_ptr), sz);
        
      } else {
        
        Grappa::FullEmpty<void*> result(addr);
        result.reset();
        auto g_result = make_global(&result);
        
        Grappa::send_heap_message(src_core, [src_ptr,sz,g_result]{
          
          void* ptr = resolve_ptr<GPTR>(src_ptr);
          
          if (Grappa::impl::locale_shared_memory.is_valid_address(ptr)) {
            
            Grappa::send_heap_message(g_result.core(), [g_result](void * payload, size_t psz){
              
              auto r = g_result.pointer();
              auto dest_addr = r->readXX();
              memcpy(dest_addr, payload, psz);
              r->writeEF(dest_addr);
              
            }, ptr, sz);
            
          } else {
            
            auto out_buf = Grappa::locale_alloc<int8_t>(sz); // <-- this is slow
            memcpy(out_buf, reinterpret_cast<int8_t*>(ptr), sz);
            
            auto msg = Grappa::heap_message(g_result.core(), [g_result](void* p, size_t psz){
              
              auto addr = g_result->readXX();
              memcpy(addr, p, psz);
              g_result->writeEF(nullptr);
              
            }, out_buf, sz);
            msg->delete_payload_after_send();
            msg->enqueue();
            
          }
        });
        
        result.readFF();
      }
    }
  
    template< bool GPTR >
    void do_put(Core dest_core, void *dest, void *src, size_t sz) {
      auto origin = Grappa::mycore();
      
      if (dest_core == origin) {
        
        memcpy(resolve_ptr<GPTR>(dest), src, sz);
        
      } else {
        
        Grappa::FullEmpty<bool> result;
        auto g_result = make_global(&result);
        
        void* tmp = nullptr;
        
        if (!Grappa::impl::locale_shared_memory.is_valid_address(src)) {
          tmp = Grappa::locale_alloc<int8_t>(sz);
          memcpy(tmp, src, sz);
          src = tmp;
        }
        
        Grappa::send_heap_message(dest_core, [dest,g_result](void * p, size_t psz){
          
          memcpy(resolve_ptr<GPTR>(dest), p, psz);
          
          Grappa::send_heap_message(g_result.core(), [g_result]{ g_result->writeEF(true); });
          
        }, src, sz);
        
        result.readFF();
        if (tmp) Grappa::locale_free(tmp);
      }
    }
  }
}

/// most basic way to read data from remote address (compiler generates these from global* accesses)
extern "C"
void grappa_get(void *addr, void global* src, size_t sz) {
  auto src_core = gaddr(src).core();
  Grappa::impl::do_get<true>(addr, src_core, reinterpret_cast<void*>(src), sz);
}

extern "C"
void grappa_get_on(Core c, void *addr, void* src, size_t sz) {
  Grappa::impl::do_get<false>(addr, c, src, sz);
}


/// most basic way to write data at remote address (compiler generates these from global* accesses)
extern "C"
void grappa_put(void global* dest, void* src, size_t sz) {
  Grappa::impl::do_put<true>(gaddr(dest).core(), reinterpret_cast<void*>(dest), src, sz);
}

extern "C"
void grappa_put_on(Core c, void* dest, void* src, size_t sz) {
  Grappa::impl::do_put<false>(c, dest, src, sz);
}


using retcode_t = int16_t;

extern "C"
retcode_t grappa_on(Core dst, retcode_t (*fn)(void* args, void* out), void* args, size_t args_sz,
               void* out, size_t out_sz) {

  auto origin = Grappa::mycore();
  
  retcode_t retcode;
  
  if (dst == origin) {
    VLOG(5) << "short-circuit";
    delegate_short_circuits++;

    retcode = fn(args, out);
        
  } else if ( args_sz <= SMALL_MSG_SIZE && out_sz <= SMALL_MSG_SIZE) {
    VLOG(5) << "small_msg";
    delegate_ops_small_msg++;
    
    int8_t _args[SMALL_MSG_SIZE];
    int8_t _out[SMALL_MSG_SIZE];
    void* out_buf = _out;
    
    memcpy(_args, args, args_sz);
    
    Grappa::FullEmpty<retcode_t> fe;
    auto gfe = make_global(&fe);
    
    Grappa::send_heap_message(dst, [fn,_args,gfe,out_buf]{
      
      int8_t _out[SMALL_MSG_SIZE];
      
      retcode_t retcode = fn((void*)_args, _out);
      
      Grappa::send_heap_message(gfe.core(), [gfe,_out,out_buf,retcode]{
        
        memcpy(out_buf, _out, SMALL_MSG_SIZE);
        gfe->writeEF(retcode);
      });
      
    });
    
    retcode = fe.readFF();
    
    memcpy(out, _out, out_sz);
    
  } else {
    delegate_ops_payload_msg++;
    
    Grappa::FullEmpty<retcode_t> fe;
    auto gfe = make_global(&fe);
    
    Grappa::send_heap_message(dst, [fn,out,out_sz,gfe](void* args, size_t args_sz){
      
      auto out_buf = Grappa::locale_alloc<int8_t>(out_sz); // <-- this is slow
      retcode_t retcode = fn(args, out_buf);
      
      auto msg = Grappa::heap_message(gfe.core(), [out,gfe,retcode](void* p, size_t psz){
        
        memcpy(out, p, psz);
        gfe->writeEF(retcode);
        
      }, out_buf, out_sz);
      msg->delete_payload_after_send();
      msg->enqueue();
      
    }, args, args_sz);
    
    retcode = fe.readFF();
  }
  return retcode;
}

#define GRAPPA_ON_ASYNC_SIZE(SIZE) \
extern "C" \
retcode_t grappa_on_async_##SIZE(Core dst, retcode_t (*fn)(void* args, void* out), void* args, size_t args_sz, Grappa::GlobalCompletionEvent * gce) { \
  gce->enroll(); \
  auto g_gce = make_global(gce); \
  delegate_ops_small_msg++; \
  int8_t _args[SIZE]; \
  memcpy(_args, args, args_sz); \
  Grappa::send_heap_message(dst, [fn,_args,g_gce]{ \
    fn((void*)_args, nullptr); \
    g_gce->send_completion(g_gce.core()); \
  }); \
  return 0; \
}

GRAPPA_ON_ASYNC_SIZE(16);
GRAPPA_ON_ASYNC_SIZE(32);
GRAPPA_ON_ASYNC_SIZE(48);
GRAPPA_ON_ASYNC_SIZE(64);
GRAPPA_ON_ASYNC_SIZE(128);

extern "C"
retcode_t grappa_on_async(Core dst, retcode_t (*fn)(void* args, void* out), void* args, size_t args_sz, Grappa::GlobalCompletionEvent * gce) {
  
  gce->enroll();
  auto g_gce = make_global(gce);
  
  if (dst == Grappa::mycore()) {
    VLOG(5) << "short-circuit";
    delegate_short_circuits++;
    
    fn(args, nullptr);
    gce->complete();
    
#define SMALL_MSG_CASE(SIZE) \
  } else if ( args_sz <= SIZE ) { \
    VLOG(5) << "small_msg"; \
    delegate_ops_small_msg++; \
    int8_t _args[SIZE]; \
    memcpy(_args, args, args_sz); \
    Grappa::send_heap_message(dst, [fn,_args,g_gce]{ \
      fn((void*)_args, nullptr); \
      g_gce->send_completion(g_gce.core()); \
    });
    
  SMALL_MSG_CASE(16)
  SMALL_MSG_CASE(32)
  SMALL_MSG_CASE(48)
  SMALL_MSG_CASE(64)

#undef SMALL_MSG_CASE
    
  } else {
    delegate_ops_payload_msg++;
    
    Grappa::send_heap_message(dst, [fn,g_gce](void* args, size_t args_sz){
      fn(args, nullptr);
      g_gce->send_completion(g_gce.core());
    }, args, args_sz);
    
  }
  
  return 0;
}

struct delegate_fetch_add_args {
  long global* addr;
  long increment;
};
struct delegate_fetch_add_out {
  long before_val;
};

void delegate_fetch_add(void *args_v, void *out_v) {
  auto *args = static_cast<delegate_fetch_add_args*>(args_v);
  auto *out = static_cast<delegate_fetch_add_out*>(out_v);
  long *p = Grappa::pointer(args->addr);
  out->before_val = *p;
  *p += args->increment;
}
