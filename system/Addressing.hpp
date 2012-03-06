
#ifndef __ADDRESSING_HPP__
#define __ADDRESSING_HPP__

/// Global Addresses for SoftXMT
///
///
/// 
///
/// We support two types of global addresses:
/// -# 2D addresses
/// -# Linear addresses 

#include "SoftXMT.hpp"

typedef int Pool;

/// assumes user data will have the top 16 bits all 0.

static const int block_size = sizeof(int64_t) * 8;

static const int tag_bits = 1;
static const int pointer_bits = 48;
static const int node_bits = 64 - pointer_bits - tag_bits;
static const int pool_bits = 64 - pointer_bits - tag_bits;

static const int tag_shift_val = 64 - tag_bits;
static const int pointer_shift_val = 64 - pointer_bits;
static const int node_shift_val = pointer_bits;
static const int pool_shift_val = pointer_bits;

static const intptr_t tag_mask = (1L << tag_shift_val);
static const intptr_t node_mask = (1L << node_bits) - 1;
static const intptr_t pool_mask = (1L << pool_bits) - 1;
static const intptr_t pointer_mask = (1L << pointer_bits) - 1;

template< typename T >
class GlobalAddress {
private:
  intptr_t storage_;
  //DISALLOW_COPY_AND_ASSIGN( GlobalAddress );
  template< typename U > friend std::ostream& operator<<( std::ostream& o, const GlobalAddress< U >& ga );
  //friend template< typename U > GlobalAddress< U > operator+( const GlobalAddress< U >& t, ptrdiff_t i );
  template< typename U > friend ptrdiff_t operator-( const GlobalAddress< U >& t, const GlobalAddress< U >& u );


  std::ostream& dump( std::ostream& o ) const {
    if( is_2D() ) {
      return o << "<GA 2D " << (void*)storage_ 
               << ": node " << node() 
               << " pointer " << pointer() 
               << ">";
    } else {
        return o << "<GA Linear " << (void*)storage_ 
                 << ": pool " << pool() 
                 << " pointer " << pointer() 
                 << ">";
    }
  }

  // GlobalAddress( T * p, Node n = SoftXMT_mynode() )
  //   : storage_( ( 1L << tag_shift_val ) |
  //               ( ( n & node_mask) << node_shift_val ) |
  //               ( reinterpret_cast<intptr_t>( p ) ) )
  // {
  //   assert( SoftXMT_mynode() <= node_mask ); 
  //   assert( reinterpret_cast<intptr_t>( p ) >> node_shift_val == 0 );
  // }

public:

  GlobalAddress( ) : storage_( 0 ) { }

  /// construct a 2D global address
  static GlobalAddress TwoDimensional( T * t, Node n = SoftXMT_mynode() )
  {
    GlobalAddress g;
    g.storage_ = ( ( 1L << tag_shift_val ) |
                   ( ( n & node_mask) << node_shift_val ) |
                   ( reinterpret_cast<intptr_t>( t ) ) );
    assert( SoftXMT_mynode() <= node_mask ); 
    assert( reinterpret_cast<intptr_t>( t ) >> node_shift_val == 0 );
    return g;
  }

  /// construct a linear global address
  static GlobalAddress Linear( T * t, Pool p = 0 )
  {
    GlobalAddress g;
    g.storage_ = ( ( 0L << tag_shift_val ) |
                   //( ( n & node_mask) << node_shift_val ) |
                   ( reinterpret_cast<intptr_t>( t ) ) );
    assert( reinterpret_cast<intptr_t>( t ) >> node_shift_val == 0 );
    return g;
  }

  /// construct a global address from raw bits
  static GlobalAddress Raw( intptr_t t )
  {
    GlobalAddress g;
    g.storage_ = t;
    return g;
  }

  inline intptr_t raw_bits() const {
    return storage_;
  }

  inline Node node() const {
    if( is_2D() ) {
      return (storage_ >> node_shift_val) & node_mask;
    } else {
      intptr_t signextended = (storage_ << pointer_shift_val) >> pointer_shift_val;
      intptr_t offset = signextended % block_size;
      intptr_t node = (signextended / block_size) % SoftXMT_nodes();
      intptr_t block = (signextended / block_size) / SoftXMT_nodes();
      return node;
    }
  }
    
  inline Pool pool() const {
    return (storage_ >> pool_shift_val) & pool_mask;
  }
    

  inline T * pointer() const { 
    if( is_2D() ) {
      intptr_t signextended = (storage_ << pointer_shift_val) >> pointer_shift_val;
      return reinterpret_cast< T * >( signextended ); 
    } else {
      intptr_t signextended = (storage_ << pointer_shift_val) >> pointer_shift_val;
      intptr_t offset = signextended % block_size;
      intptr_t node = (signextended / block_size) % SoftXMT_nodes();
      intptr_t block = (signextended / block_size) / SoftXMT_nodes();
      return reinterpret_cast< T * >( block * block_size + offset );
    }
  }

  inline bool is_2D() const {
    return storage_ & tag_mask; 
  }

  inline bool is_linear() const {
    return !( storage_ & tag_mask );
  }

  inline GlobalAddress< T >& operator++() { 
    storage_ += sizeof(T); 
    return *this; 
  }

  //inline GlobalAddress< T > operator++(int i) { return storage_ ++ i; }

  inline GlobalAddress< T >& operator--() { 
    storage_ -= sizeof(T); 
    return *this; 
  }
  
  //inline GlobalAddress< T > operator--(int i) { return storage_ ++ i; }

  inline GlobalAddress< T >& operator+=(ptrdiff_t i) { 
    storage_ += i * sizeof(T); 
    return *this; 
  }

  inline GlobalAddress< T >& operator-=(ptrdiff_t i) { 
    storage_ -= i * sizeof(T); 
    return *this; 
  }

  //T& operator[]( ptrdiff_t index ) { return 

  template< typename U >
  operator GlobalAddress< U >( ) {
    GlobalAddress< U > u = GlobalAddress< U >::Raw( storage_ );
    return u;
  }

  template< typename U >
  operator U * ( ) {
    U * u = reinterpret_cast< U * >( storage_ );
    return u;
  }


};


template< typename T >
GlobalAddress< T > operator+( const GlobalAddress< T >& t, ptrdiff_t i ) {
  GlobalAddress< T > tt(t);
  return tt += i;
}

// template< typename T >
// GlobalAddress< T >&& operator+( GlobalAddress< T >&& t, ptrdiff_t i ) {
//   return t += i;
// }

template< typename T >
GlobalAddress< T > operator-( const GlobalAddress< T >& t, ptrdiff_t i ) {
  GlobalAddress< T > tt(t);
  return tt -= i;
}

// template< typename T >
// GlobalAddress< T >&& operator-( const GlobalAddress< T >&& t, ptrdiff_t i ) {
//   return t -= i;
// }

template< typename T >
ptrdiff_t operator-( const GlobalAddress< T >& t, const GlobalAddress< T >& u ) {
  return t - u;
}

template< typename T >
GlobalAddress< T > localToGlobal( T * t ) {
  return GlobalAddress< T >::TwoDimensional( t, SoftXMT_mynode() );
}

template< typename T >
GlobalAddress< T > make_global( T * t, Node n = SoftXMT_mynode() ) {
  return GlobalAddress< T >::TwoDimensional( t, n );
}

/// takes a local pointer to a block-cyclic distributed chuck of
/// memory allocated at the same base address on all nodes, and makes
/// a linear global pointer pointing to that byte.
template< typename T >
GlobalAddress< T > make_linear( T * t ) {
  intptr_t tt = reinterpret_cast< intptr_t >( t );
  
  intptr_t offset = tt % block_size;
  intptr_t block = tt / block_size;
  intptr_t node = block % SoftXMT_nodes();
  intptr_t ga = ( block * SoftXMT_nodes() + node ) * block_size + offset;

  T * ttt = reinterpret_cast< T * >( ga );
  GlobalAddress< T > tttt = GlobalAddress< T >::Linear( ttt, 0 );

  CHECK_EQ( tttt.node(), node ) << "converted linear address node doesn't match";
  CHECK_EQ( tttt.pointer(), t ) << "converted linear address local pointer doesn't match";

  return tttt;
}

template< typename T >
std::ostream& operator<<( std::ostream& o, const GlobalAddress< T >& ga ) {
  return ga.dump( o );
}

//template< typename T >
#endif
