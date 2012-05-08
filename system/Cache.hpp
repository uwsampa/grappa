
#ifndef __CACHE_HPP__
#define __CACHE_HPP__

#include "SoftXMT.hpp"
#include "Addressing.hpp"
#include "common.hpp"

#include <glog/logging.h>

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
  {
    VLOG(6) << "buffer = " << buffer << ", storage_ = " << storage_;
  }
  ~CacheAllocator() {
    if( heap_ && storage_ != NULL ) {
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
class NullAcquirer {
private:
  GlobalAddress<T> request_address_;
  size_t count_;
  T ** pointer_;
public:
  NullAcquirer(GlobalAddress<T> request_address, size_t count, T** pointer)
  : request_address_(request_address), count_(count), pointer_(pointer)
  {
    VLOG(6) << "pointer = " << pointer << ", pointer_ = " << pointer_;
    if( request_address_.is_2D() && request_address_.node() == SoftXMT_mynode() ) {
      DVLOG(5) << "Short-circuiting to address " << request_address_.pointer();
      *pointer_ = request_address_.pointer();
    }
  }
  void start_acquire() {}
  void block_until_acquired() {}
  bool acquired() const { return true; }
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
          template< typename TT > class Allocator, 
          template< typename TT > class Acquirer, 
          template< typename TT > class Releaser >
class CacheRO {
protected:
  GlobalAddress< T > address_;
  size_t count_;
  Allocator< T > storage_;
  T * pointer_;
  Acquirer< T > acquirer_;
  Releaser< T > releaser_;

public:
  explicit CacheRO( GlobalAddress< T > address, size_t count, T * buffer = NULL )
    : address_( address )
    , count_( count )
    , storage_( buffer, count )
    , pointer_( storage_.pointer() )
    , acquirer_( address, count, &pointer_ )
    , releaser_( address, count, &pointer_ )
  { }

  void start_acquire( ) { 
    acquirer_.start_acquire( );
  }
  void block_until_acquired() {
    acquirer_.block_until_acquired();
  }
  void start_release() { 
    releaser_.start_release( );
  }
  void block_until_released() {
    releaser_.block_until_released( );
  }
  operator const T*() { 
    block_until_acquired();
    DVLOG(5) << "Const dereference of " << address_ << " * " << count_;
    return pointer_;
  } 
  operator const void*() { 
    block_until_acquired();
    DVLOG(5) << "Const void * dereference of " << address_ << " * " << count_;
    return pointer_;
  } 
};

template< typename T, 
          template< typename TT > class Allocator, 
          template< typename TT > class Acquirer, 
          template< typename TT > class Releaser >
class CacheRW {
protected:
  GlobalAddress< T > address_;
  size_t count_;
  Allocator< T > storage_;
  T * pointer_;
  Acquirer< T > acquirer_;
  Releaser< T > releaser_;

public:
  explicit CacheRW( GlobalAddress< T > address, size_t count, T * buffer = NULL )
    : address_( address )
    , count_( count )
    , storage_( buffer, count )
    , pointer_( storage_.pointer() )
    , acquirer_( address, count, &pointer_ )
    , releaser_( address, count, &pointer_ )
  { }

  ~CacheRW() {
    block_until_released();
  }

  void start_acquire( ) { 
    acquirer_.start_acquire( );
  }
  void block_until_acquired() {
    acquirer_.block_until_acquired();
  }
  void start_release() { 
    releaser_.start_release( );
  }
  void block_until_released() {
    releaser_.block_until_released( );
  }
  operator T*() { 
    block_until_acquired();
    DVLOG(5) << "RW dereference of " << address_ << " * " << count_;
    return pointer_;
  } 
  operator void*() { 
    block_until_acquired();
    DVLOG(5) << "RW dereference of " << address_ << " * " << count_;
    return pointer_;
  } 
};

template< typename T, 
template< typename TT > class Allocator, 
template< typename TT > class Acquirer, 
template< typename TT > class Releaser >
class CacheWO {
protected:
  GlobalAddress< T > address_;
  size_t count_;
  Allocator< T > storage_;
  T * pointer_;
  Acquirer< T > acquirer_;
  Releaser< T > releaser_;
  
public:
  explicit CacheWO( GlobalAddress< T > address, size_t count, T * buffer = NULL )
  : address_( address )
  , count_( count )
  , storage_( buffer, count )
  , pointer_( storage_.pointer() )
  , acquirer_( address, count, &pointer_ )
  , releaser_( address, count, &pointer_ )
  {
    VLOG(6) << "pointer_ = " << pointer_ << ", &pointer_ = " << &pointer_ << ", storage_.pointer() = " << storage_.pointer();
  }
  
  ~CacheWO() {
    block_until_released();
  }
  
  void start_acquire( ) {
    acquirer_.start_acquire( );
  }
  void block_until_acquired() {
    acquirer_.block_until_acquired();
  }
  void start_release() { 
    releaser_.start_release( );
  }
  void block_until_released() {
    releaser_.block_until_released( );
  }
  operator T*() { 
    block_until_acquired();
    DVLOG(5) << "WO dereference of " << address_ << " * " << count_;
    VLOG(1) << "pointer_ = " << pointer_;
    return pointer_;
  } 
  operator void*() { 
    block_until_acquired();
    DVLOG(5) << "WO dereference of " << address_ << " * " << count_;
    return pointer_;
  }
};
    
template< typename T >
struct Incoherent {
  typedef CacheRO< T, CacheAllocator, IncoherentAcquirer, NullReleaser > RO;
   typedef CacheRW< T, CacheAllocator, IncoherentAcquirer, IncoherentReleaser > RW;
  typedef CacheWO< T, CacheAllocator, NullAcquirer, IncoherentReleaser > WO;
};


///
/// Wrapper functions for making it simpler to use the bare-bones tasking
///

/// Cache GlobalAddress argument and pass pointer to cached copy to the wrapped function
//template< typename T, void (*F)(T*) >
//void call_with_caching(GlobalAddress<T> ptr) {
//    VLOG(5) << "caching args";
//    T buf;
//    typename Incoherent<T>::RW cache(ptr, 1, &buf);
//    cache.block_until_acquired();
//    F(&cache[0]);
//    cache.block_until_released();
//}
//
///// Cache GlobalAddress argument and pass cached copy as a ref to the wrapped function
//template< typename T, void (*F)(T&) >
//void call_with_caching(GlobalAddress<T> ptr) {
//    VLOG(5) << "caching args";
//    T buf;
//    typename Incoherent<T>::RW cache(ptr, 1, &buf);
//    cache.block_until_acquired();
//    F(*cache);
//    cache.block_until_released();
//}

/// Cache GlobalAddress argument and pass pointer to const cached copy to the wrapped function
template< typename T, void (*F)(const T*) >
void call_with_caching(GlobalAddress<T> ptr) {
    VLOG(5) << "caching args";
    T buf;
    typename Incoherent<T>::RO cache(ptr, 1, &buf);
    cache.block_until_acquired();
    F(&cache[0]);
    cache.block_until_released();
}

/// Cache GlobalAddress argument and pass const cached copy as a ref to the wrapped function
template< typename T, void (*F)(const T&) >
void call_with_caching(GlobalAddress<T> ptr) {
    VLOG(5) << "caching args";
    T buf;
    typename Incoherent<T>::RO cache(ptr, 1, &buf);
    cache.block_until_acquired();
    F(*cache);
    cache.block_until_released();
}


/// Wrap arguments to spawn task (at call site) for tasks that used to assume caching
#define CACHE_WRAP(fn, arg) \
&call_with_caching<typename boost::remove_pointer<BOOST_TYPEOF(arg)>::type,fn>, make_global(arg)

/// Declare a version of a task function that accepts a GlobalAddress to the arguments
/// and does the caching for you.
#define DECLARE_CACHE_WRAPPED(name, orig, T) \
inline void name(GlobalAddress<T> a) { return call_with_caching<T,orig>(a); }
  

#endif
