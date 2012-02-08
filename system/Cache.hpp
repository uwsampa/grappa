
#ifndef __CACHE_HPP__
#define __CACHE_HPP__

#include "SoftXMT.hpp"
#include "Addressing.hpp"
#include "common.hpp"

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
};

template< typename T >
class IncoherentAcquirer {
private:
  bool acquire_started_;
  bool acquired_;
  GlobalAddress< T > request_address_;
  size_t count_;
  void * storage_;
public:
  IncoherentAcquirer( GlobalAddress< T > request_address, size_t count, void * storage )
    : request_address_( request_address )
    , count_( count )
    , storage_( storage )
  { }
    
  void start_acquire();
  void block_until_acquired();
  bool acquired() const { return acquired_; }

  struct ReplyArgs {
    GlobalAddress< IncoherentAcquirer > reply_address;
  };

  struct RequestArgs {
    GlobalAddress< T > request_address;
    size_t count;
    GlobalAddress< IncoherentAcquirer< T > > reply_address;
  };


};


// class IncoherentReleaser {
// private:
//   bool release_started_;
//   bool released_;
//   void * request_address_;
//   size_t size_;
//   void * storage_;

// public:
//   IncoherentReleaser( void * request_address, size_t size, void * storage )
//     : request_address_( request_address )
//     , size_( size )
//     , storage_( storage )
//   { }
//   void start_release();
//   void block_until_released();
//   bool released() const { return released_; }
// };

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
class Cache {
protected:
  GlobalAddress< T > address_;
  size_t count_;
  Allocator< T > storage_;
  Acquirer< T > acquirer_;
  Releaser< T > releaser_;

public:
  Cache( GlobalAddress< T > address, size_t count = 1, T * buffer = NULL )
    : address_( address )
    , count_( count )
    , storage_( buffer, count )
    , acquirer_( address, count, (void *)storage_ )
    , releaser_( address, count, (void *)storage_ )
  { 
    for( int i = 0; i < count; ++i ) {
      storage_[i] = 0;
    }
  }

  void start_acquire( ) { 
    acquirer_.start_acquire( ); //ga, count, &data );
  }
  void block_until_acquired() {
    acquirer_.block_until_acquired( );
  }
  void start_release() { 
    releaser_.start_release( );
  }
  void block_until_released() {
    releaser_.block_until_released( );
  }
};

template< typename T, 
          template< typename T > class Allocator, 
          template< typename T > class Acquirer, 
          template< typename T > class Releaser >
class CacheRO : public Cache< T, Allocator, Acquirer, Releaser > {
private:
  typedef Cache< T, Allocator, Acquirer, Releaser > Parent;
public:
  CacheRO( GlobalAddress< T > address, size_t count = 1, T * buffer = NULL )
    : Parent( address, count, buffer )
  { }
  inline operator T*() const { 
    return this->storage_; 
  } 
  inline operator void*() const { 
    return this->storage_;
  } 
};

// template< typename T, 
//           template< typename T > class Allocator, 
//           template< typename T > class Acquirer, 
//           template< typename T > class Releaser >
// class CacheRW : public Cache< T, Allocator, Acquirer, Releaser > {
// private:
//   typedef Cache< T, Allocator, Acquirer, Releaser > Parent;
// public:
//   CacheRW( GlobalAddress< T > address, size_t count = 1, T * buffer = NULL )
//     : Parent( address, count, buffer )
//   { }
//   inline operator T*() { 
//     return this->storage_; 
//   } 
//   inline operator void*() { 
//     return this->storage_;
//   } 
// };

template< typename T >
struct Incoherent {
  typedef CacheRO< T, CacheAllocator, IncoherentAcquirer, NullReleaser > RO;
  //  typedef CacheRW< T, CacheAllocator, IncoherentAcquirer, IncoherentReleaser > RW;
};


#endif
