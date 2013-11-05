#include <Addressing.hpp>
#include <Communicator.hpp>
#include <Message.hpp>
#include <string.h>

/// define 'global' pointer annotation
#define global __attribute__((address_space(100)))


bool ga_is_2D(void global* g) {
  return reinterpret_cast<intptr_t>(g) & tag_mask;
}

Core ga_core(void global* g) {
  auto storage_ = reinterpret_cast<intptr_t>(g);
  if( ga_is_2D(g) ) {
    return (storage_ >> node_shift_val) & node_mask;
  } else {
    return (storage_ / block_size) % global_communicator.nodes();
  }
}

template< typename T >
inline T* ga_pointer(T global* g) {
  auto storage_ = reinterpret_cast<intptr_t>(g);
  if( ga_is_2D(g) ) {
    intptr_t signextended = (storage_ << pointer_shift_val) >> pointer_shift_val;
    return reinterpret_cast< T* >( signextended );
  } else {
    intptr_t offset = storage_ % block_size;
    intptr_t node_block = (storage_ / block_size) / global_communicator.nodes();
    intptr_t address = node_block * block_size + offset
                       + reinterpret_cast< intptr_t >( Grappa::impl::global_memory_chunk_base );
    return reinterpret_cast< T* >( address );
  }
}

template< typename T >
inline T global* ga_make_global( T* t, Core n = Grappa::mycore() ) {
  auto g = ( ( 1L << tag_shift_val ) |
                ( ( n & node_mask) << node_shift_val ) |
                ( reinterpret_cast<intptr_t>( t ) ) );
  assert( global_communicator.mynode() <= node_mask );
  assert( reinterpret_cast<intptr_t>( t ) >> node_shift_val == 0 );
  return reinterpret_cast<T global*>(g);
}

/// most basic way to get data from global address
///
/// TODO: have aggregator implement this directly to save overhead (i.e. instead of serializing/deserializing it can just extract the pointers directly)
extern "C"
void grappa_get(void *addr, void global* g, size_t sz) {
  
  auto origin = Grappa::mycore();
  auto dest = ga_core(g);
  
  if (dest == origin) {
    
    memcpy(addr, ga_pointer(g), sz);
    
  } else {
    
    Grappa::FullEmpty<void*> result(addr); result.reset();
    
    auto g_result = ga_make_global(&result);
    
    Grappa::send_message(ga_core(g), [g,sz,g_result]{
      
      Grappa::send_heap_message(ga_core(g_result), [g_result](void * payload, size_t psz){
        
        auto r = ga_pointer(g_result);
        auto dest_addr = r->readXX();
        memcpy(dest_addr, payload, psz);
        r->writeEF(dest_addr);
        
      }, ga_pointer(g), sz);
      
    });
    
    result.readFF();
  }
}

void grappa_foo() { printf("foo\n"); }
