
#ifndef __ADDRESSING_HPP__
#define __ADDRESSING_HPP__

#include "SoftXMT.hpp"

/// assumes user data will have the top 16 bits all 0.

static const int tag_bits = 1;
static const int pointer_bits = 48;
static const int node_bits = 64 - pointer_bits - tag_bits;

static const int tag_shift_val = 64 - tag_bits;
static const int pointer_shift_val = 64 - pointer_bits;
static const int node_shift_val = pointer_bits;

static const intptr_t node_mask = (1L << node_bits) - 1;
static const intptr_t pointer_mask = (1L << pointer_bits) - 1;

template< typename T >
class GlobalPointer {
private:
  intptr_t storage;

public:

  GlobalPointer( T * p )
    : storage( ( 1L << tag_shift_val ) |
               ( (SoftXMT_mynode() & node_mask) << node_shift_val ) |
               ( reinterpret_cast<intptr_t>( p ) ) )
  {
    assert( SoftXMT_mynode() <= node_mask ); 
    assert( reinterpret_cast<intptr_t>( p ) >> node_shift_val == 0 );
  }

  inline Node node() const {
    return (storage >> node_shift_val) & node_mask;
  }
    

  inline T * pointer() const { 
    intptr_t signextended = (storage << pointer_shift_val) >> pointer_shift_val;
    return reinterpret_cast< T * >( signextended ); 
  }

};

template< typename T >
GlobalPointer< T > localToGlobal( T * t ) {
  return GlobalPointer< T >( t );
}



//template< typename T >
#endif
