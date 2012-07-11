
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
///
/// 2D addresses are node, address on node
///
/// linear addresses are block cyclic

#include "Communicator.hpp"

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
               << " pointer " << static_cast<void *>( pointer() )
               << ">";
    } else {
        return o << "<GA Linear " << (void*)storage_ 
                 << ": pool " << pool() 
                 << " node " << node() 
                 << " pointer " << static_cast<void *>( pointer()  )
                 << ">";
    }
  }

  // GlobalAddress( T * p, Node n = global_communicator.mynode() )
  //   : storage_( ( 1L << tag_shift_val ) |
  //               ( ( n & node_mask) << node_shift_val ) |
  //               ( reinterpret_cast<intptr_t>( p ) ) )
  // {
  //   assert( global_communicator.mynode() <= node_mask );
  //   assert( reinterpret_cast<intptr_t>( p ) >> node_shift_val == 0 );
  // }

public:

  GlobalAddress( ) : storage_( 0 ) { }

  /// construct a 2D global address
  static GlobalAddress TwoDimensional( T * t, Node n = global_communicator.mynode() )
  {
    GlobalAddress g;
    g.storage_ = ( ( 1L << tag_shift_val ) |
                   ( ( n & node_mask) << node_shift_val ) |
                   ( reinterpret_cast<intptr_t>( t ) ) );
    assert( global_communicator.mynode() <= node_mask );
    assert( reinterpret_cast<intptr_t>( t ) >> node_shift_val == 0 );
    return g;
  }

  /// construct a linear global address
  static GlobalAddress Linear( T * t, Pool p = 0 )
  {
    intptr_t tt = reinterpret_cast< intptr_t >( t );

    intptr_t offset = tt % block_size;
    intptr_t block = tt / block_size;
    // intptr_t node_from_address = block % global_communicator.nodes();
    // CHECK_EQ( node_from_address, 0 ) << "Node from address should be zero. (Check alignment?)";
    intptr_t ga = ( block * global_communicator.nodes() + global_communicator.mynode() ) * block_size + offset;
    
    T * ttt = reinterpret_cast< T * >( ga );
    
    GlobalAddress g;
    g.storage_ = ( ( 0L << tag_shift_val ) |
                   //( ( n & node_mask) << node_shift_val ) |
                   ( reinterpret_cast<intptr_t>( ttt ) ) );

    CHECK_EQ( g.node(), global_communicator.mynode() ) << "converted linear address node doesn't match";
    CHECK_EQ( g.pointer(), t ) << "converted linear address local pointer doesn't match";
    
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
      intptr_t offset = storage_ % block_size;
      intptr_t node = (storage_ / block_size) % global_communicator.nodes();
      intptr_t node_block = (storage_ / block_size) / global_communicator.nodes();
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
      intptr_t offset = storage_ % block_size;
      intptr_t block = (storage_ / block_size);
      intptr_t node = (storage_ / block_size) % global_communicator.nodes();
      intptr_t node_block = (storage_ / block_size) / global_communicator.nodes();
      return reinterpret_cast< T * >( node_block * block_size + offset );
    }
  }

  // base address of block containing this byte
  inline GlobalAddress< T > block_min() const { 
    if( is_2D() ) {
      //intptr_t signextended = (storage_ << pointer_shift_val) >> pointer_shift_val;
      //GlobalAddress< U > u = GlobalAddress< U >::Raw( storage_ );
      return GlobalAddress< T >::TwoDimensional( (T*) 0, node() );
    } else {
      intptr_t first_byte = storage_;
      intptr_t first_byte_offset = first_byte % block_size;
      intptr_t node = (first_byte / block_size) %   global_communicator.nodes();
      intptr_t node_block = (first_byte / block_size) / global_communicator.nodes();
      return GlobalAddress< T >::Raw( this->raw_bits() - first_byte_offset );
    }
  }

  inline GlobalAddress< char > first_byte() const { 
    return GlobalAddress< char >::Raw( this->raw_bits() );
  }

  inline GlobalAddress< char > last_byte() const { 
    return GlobalAddress< char >::Raw( this->raw_bits() + sizeof(T) - 1 );
  }

  // base address of block after the one containing the last byte of this object
  inline GlobalAddress< T > block_max() const { 
    if( is_2D() ) {
      intptr_t signextended = (storage_ << pointer_shift_val) >> pointer_shift_val;
      //return reinterpret_cast< T * >( -1 ); 
      return GlobalAddress< T >::TwoDimensional( (T*) 1, node() );
    } else {
      intptr_t first_byte = storage_;
      intptr_t first_byte_offset = first_byte % block_size;
      intptr_t last_byte = first_byte + sizeof(T) - 1;
      intptr_t last_byte_offset = last_byte % block_size;
      intptr_t node = (last_byte / block_size) % global_communicator.nodes();
      intptr_t node_block = (last_byte / block_size) / global_communicator.nodes();
      return GlobalAddress< T >::Raw( this->raw_bits() + sizeof(T) + block_size - (last_byte_offset + 1) );
    }
  }

  inline bool is_2D() const {
    return storage_ & tag_mask; 
  }

  inline bool is_linear() const {
    return !( storage_ & tag_mask );
  }

  // inline size_t block_size() const {
  //   return block_size_;
  // }

  inline GlobalAddress< T >& operator++() {
    storage_ += sizeof(T); 
    return *this;
  }
  
  inline GlobalAddress< T > operator++(int) {
    GlobalAddress<T> result = *this;
    storage_ += sizeof(T);
    return result;
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

  bool equals( const GlobalAddress< T >& t ) const {
    return raw_bits() == t.raw_bits();
  }

  bool operator==( const GlobalAddress< T >& t ) const {
    return raw_bits() == t.raw_bits();
  }

  template< typename U >
  bool operator==( const GlobalAddress< U >& u ) const {
    return raw_bits() == u.raw_bits();
  }
  
  bool operator<(const GlobalAddress<T>& t) const {
    return raw_bits() < t.raw_bits();
  }

  //T& operator[]( ptrdiff_t index ) { return 

  //TODO: do we really need this? leads to unneccessary type errors...
  template< typename U >
  operator GlobalAddress< U >( ) {
    GlobalAddress< U > u = GlobalAddress< U >::Raw( storage_ );
    return u;
  }

  operator void * ( ) {
    void * u = reinterpret_cast< void * >( storage_ );
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
inline ptrdiff_t operator-( const GlobalAddress< T >& t, const GlobalAddress< T >& u ) {
  LOG(WARNING) << "Danger! You probably don't want to call this function. " 
	       << "Try using .first_byte() or .last_byte() to get the right overload";
  return t.raw_bits() - u.raw_bits();
}

template<  >
inline ptrdiff_t operator-<char>( const GlobalAddress< char >& t, const GlobalAddress< char >& u ) {
  return t.raw_bits() - u.raw_bits();
}

template< typename T >
GlobalAddress< T > localToGlobal( T * t ) {
  return GlobalAddress< T >::TwoDimensional( t, global_communicator.mynode() );
}

template< typename T >
GlobalAddress< T > make_global( T * t, Node n = global_communicator.mynode() ) {
  return GlobalAddress< T >::TwoDimensional( t, n );
}

/// takes a local pointer to a block-cyclic distributed chuck of
/// memory allocated at the same base address on all nodes, and makes
/// a linear global pointer pointing to that byte.
template< typename T >
GlobalAddress< T > make_linear( T * t ) {
  return GlobalAddress< T >::Linear( t );
}


template< typename T >
std::ostream& operator<<( std::ostream& o, const GlobalAddress< T >& ga ) {
  return ga.dump( o );
}

/// computes offsets of members in structs and claases
/// call like this:
///   struct Foo {
///     int i;
///   } foo;
///   GlobalAddress< Foo > foo_gp = make_global( foo );
///   GlobalAddress< int > foo_i_gp = global_pointer_to_member( foo_gp, &Foo::i );
template< typename T, typename M >
inline GlobalAddress< M > global_pointer_to_member( const GlobalAddress< T > t, const M T::*m ) {
  const intptr_t t_raw = t.raw_bits();
  const T * tp = reinterpret_cast< T * >( t_raw );
  const M * mp = &(tp->*m);
  return GlobalAddress< M >::Raw( reinterpret_cast< intptr_t >( mp ) );
}


//template< typename T >
#endif
