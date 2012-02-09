
#ifndef __CACHE_HPP__
#define __CACHE_HPP__

#include "SoftXMT.hpp"
#include "Addressing.hpp"
#include "common.hpp"

#define DEBUG_CACHE 0

#include "IncoherentAcquirer.hpp"
#include "IncoherentReleaser.hpp"


template< typename T >
class CacheAllocator {
public:
  T * storage_;
  bool heap_;
  CacheAllocator( T * buffer, size_t size ) 
    : storage_( buffer != NULL ? buffer : new T[size] )
    , heap_( buffer != NULL ? false : true ) 
  { }
  ~CacheAllocator() {
    if( heap_ ) {
      delete [] storage_;
    }
  }
  operator T*() { 
    return storage_;
  }
  operator T*() const { 
    return storage_;
  }
  T * pointer() { 
    return storage_;
  }
  const T * const_pointer() const { 
    return storage_;
  }
};

template< typename T >
class NullReleaser {
private:
  bool released_;
public:
  NullReleaser( GlobalAddress< T > request_address, size_t count, void * storage )
    : released_(false) 
  { }
  void start_release() { 
    released_ = true;
  }
  void block_until_released() {
    released_ = true;
  }
  bool released() {
    return released_;
  }
};

template< typename T, 
          template< typename T > class Allocator, 
          template< typename T > class Acquirer, 
          template< typename T > class Releaser >
class CacheRO {
protected:
  GlobalAddress< T > address_;
  size_t count_;
  Allocator< T > storage_;
  Acquirer< T > acquirer_;
  Releaser< T > releaser_;

public:
  explicit CacheRO( GlobalAddress< T > address, size_t count, T * buffer = NULL )
    : address_( address )
    , count_( count )
    , storage_( buffer, count )
    , acquirer_( address, count, storage_ )
    , releaser_( address, count, storage_ )
  { }

  void start_acquire( ) { 
    if (DEBUG_CACHE) std::cout << "start_acquire" << std::endl;
    acquirer_.start_acquire( ); //ga, count, &data );
  }
  void block_until_acquired() {
    if (DEBUG_CACHE) std::cout << "block_until_acquired" << std::endl;
    acquirer_.block_until_acquired(); // TODO: why broke?
  }
  void start_release() { 
    releaser_.start_release( );
  }
  void block_until_released() {
    releaser_.block_until_released( );
  }
  operator const T*() { 
    block_until_acquired();
    return storage_.const_pointer(); 
  } 
  operator const void*() { 
    block_until_acquired();
    return storage_.const_pointer();
  } 
};

template< typename T, 
          template< typename T > class Allocator, 
          template< typename T > class Acquirer, 
          template< typename T > class Releaser >
class CacheRW {
protected:
  GlobalAddress< T > address_;
  size_t count_;
  Allocator< T > storage_;
  Acquirer< T > acquirer_;
  Releaser< T > releaser_;

public:
  explicit CacheRW( GlobalAddress< T > address, size_t count, T * buffer = NULL )
    : address_( address )
    , count_( count )
    , storage_( buffer, count )
    , acquirer_( address, count, storage_ )
    , releaser_( address, count, storage_ )
  { }

  ~CacheRW() {
    block_until_released();
  }

  void start_acquire( ) { 
    if (DEBUG_CACHE) std::cout << "start_acquire" << std::endl;
    acquirer_.start_acquire( ); //ga, count, &data );
  }
  void block_until_acquired() {
    if (DEBUG_CACHE) std::cout << "block_until_acquired" << std::endl;
    acquirer_.block_until_acquired(); // TODO: why broke?
  }
  void start_release() { 
    releaser_.start_release( );
  }
  void block_until_released() {
    releaser_.block_until_released( );
  }
  operator T*() { 
    block_until_acquired();
    return storage_.pointer(); 
  } 
  operator void*() { 
    block_until_acquired();
    return storage_.pointer();
  } 
};


template< typename T >
struct Incoherent {
  typedef CacheRO< T, CacheAllocator, IncoherentAcquirer, NullReleaser > RO;
   typedef CacheRW< T, CacheAllocator, IncoherentAcquirer, IncoherentReleaser > RW;
};


#endif
