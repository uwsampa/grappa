#include "Primitive.hpp"
#include <Message.hpp>
#include <string.h>
#include <Delegate.hpp>

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

extern "C"
void grappa_on(Core dst, void (*fn)(void* args, void* out), void* args, size_t args_sz,
               void* out, size_t out_sz) {
  auto origin = Grappa::mycore();
  
  if (dst == origin) {
    VLOG(5) << "short-circuit";
    delegate_ops_short_circuited++;
    
    fn(args, out);
    
  } else if ( args_sz < SMALL_MSG_SIZE && out_sz < SMALL_MSG_SIZE) {
    VLOG(5) << "small_msg";
    delegate_ops_small_msg++;
    
    int8_t _args[SMALL_MSG_SIZE];
    int8_t _out[SMALL_MSG_SIZE];
    
    memcpy(_args, args, args_sz);
    
    Grappa::FullEmpty<void*> fe(_out); fe.reset();
    auto gfe = make_global(&fe);
    
    Grappa::send_heap_message(dst, [fn,_args,gfe]{
      
      int8_t _out[SMALL_MSG_SIZE];
      
      fn((void*)_args, _out);
      
      Grappa::send_heap_message(gfe.core(), [gfe,_out]{
        
        auto r = gfe->readXX();
        memcpy(r, _out, SMALL_MSG_SIZE);
        gfe->writeEF(r);
        
      });
      
    });
    
    fe.readFF();
    memcpy(out, _out, out_sz);
    
  } else {
    delegate_ops_payload_msg++;
    
    Grappa::FullEmpty<void*> fe(out); fe.reset();
    auto gfe = make_global(&fe);
    
    Grappa::send_heap_message(dst, [fn,out_sz,gfe](void* args, size_t args_sz){
      
      auto out = Grappa::locale_alloc<int8_t>(out_sz); // <-- this is slow
      fn(args, out);
      
      auto msg = Grappa::heap_message(gfe.core(), [gfe](void* out, size_t out_sz){
        
        auto r = gfe->readXX();
        memcpy(r, out, out_sz);
        gfe->writeEF(r);
        
      }, out, out_sz);
      msg->delete_payload_after_send();
      msg->enqueue();
      
    }, args, args_sz);
    
    fe.readFF();
  }
}
