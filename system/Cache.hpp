////////////////////////////////////////////////////////////////////////
// This file is part of Grappa, a system for scaling irregular
// applications on commodity clusters. 

// Copyright (C) 2010-2014 University of Washington and Battelle
// Memorial Institute. University of Washington authorizes use of this
// Grappa software.

// Grappa is free software: you can redistribute it and/or modify it
// under the terms of the Affero General Public License as published
// by Affero, Inc., either version 1 of the License, or (at your
// option) any later version.

// Grappa is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// Affero General Public License for more details.

// You should have received a copy of the Affero General Public
// License along with this program. If not, you may obtain one from
// http://www.affero.org/oagpl.html.
////////////////////////////////////////////////////////////////////////

/// Main Explicit Cache API

#ifndef __CACHE_HPP__
#define __CACHE_HPP__

#include "Addressing.hpp"
#include "common.hpp"

#include <glog/logging.h>

#include "IncoherentAcquirer.hpp"
#include "IncoherentReleaser.hpp"

/// stats for caches
class CacheMetrics {
  public:
    static void count_ro_acquire( uint64_t bytes ) ; 
    static void count_wo_release( uint64_t bytes ) ; 
    static void count_rw_acquire( uint64_t bytes) ; 
    static void count_rw_release( uint64_t bytes ) ; 
};


/// Allocator for cache local storage. If you pass in a pointer to a
/// buffer you've allocated, it uses that. Otherwise, it allocates a
/// buffer.
template< typename T >
class CacheAllocator {
public:
  T * storage_;
  bool heap_;
  CacheAllocator( T * buffer, size_t size ) 
    : storage_( buffer != NULL ? buffer : reinterpret_cast< T* >
                ( Grappa::impl::locale_shared_memory.allocate( size * sizeof(T) ) ) )
    , heap_( buffer != NULL ? false : true ) 
  {
    VLOG(6) << "buffer = " << buffer << ", storage_ = " << storage_;
  }
  ~CacheAllocator() {
    if( heap_ && storage_ != NULL ) {
      Grappa::impl::locale_shared_memory.deallocate( storage_ );
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

/// No-op cache acquire behavior
template< typename T >
class NullAcquirer {
private:
  GlobalAddress<T> * request_address_;
  size_t * count_;
  T ** pointer_;
public:
  NullAcquirer(GlobalAddress<T> * request_address, size_t * count, T** pointer)
  : request_address_(request_address), count_(count), pointer_(pointer)
  {
    VLOG(6) << "pointer = " << pointer << ", pointer_ = " << pointer_;
    if( count == 0 ) {
      DVLOG(5) << "Zero-length acquire";
      *pointer_ = NULL;
    } else if( request_address_->is_2D() && request_address_->core() == Grappa::mycore() ) {
      DVLOG(5) << "Short-circuiting to address " << request_address_->pointer();
      *pointer_ = request_address_->pointer();
    }
  }
  void reset( )
  { }
  void start_acquire() {}
  void block_until_acquired() {}
  bool acquired() const { return true; }
};

/// No-op cache release behavior
template< typename T >
class NullReleaser {
private:
  bool released_;
public:
  NullReleaser( GlobalAddress< T > * request_address, size_t * count, void * storage )
    : released_(false) 
  { }
  void reset( )
  { 
    released_ = false;
  }
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

/// @addtogroup Caches
/// @{

/// Read-only cache object. This is parameterize so it can implement
/// coherent or incoherent read-only caches.
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
  /// Create a cache object. Call with a global address, a number of
  /// elements to fetch, and optionally a local buffer to store the
  /// cached copy.
  explicit CacheRO( GlobalAddress< T > address, size_t count, T * buffer = NULL )
    : address_( address )
    , count_( count )
    , storage_( buffer, count )
    , pointer_( storage_.pointer() )
    , acquirer_( &address_, &count_, &pointer_ )
    , releaser_( &address_, &count_, &pointer_ )
  { }

  /// send acquire message
  void start_acquire( ) { 
    CacheMetrics::count_ro_acquire( sizeof(T)*count_ );
    acquirer_.start_acquire( );
  }

  /// block until acquire is completed
  void block_until_acquired() {
    CacheMetrics::count_ro_acquire( sizeof(T)*count_ );
    acquirer_.block_until_acquired();
  }
  
  /// send release message
  void start_release() { 
    releaser_.start_release( );
  }

  /// block until release is completed
  void block_until_released() {
    releaser_.block_until_released( );
  }

  /// reassign cache to point at a different block
  void reset( GlobalAddress< T > address, size_t count ) {
    block_until_acquired();
    block_until_released();
    address_ = address;
    count_ = count;
    releaser_.reset( );
    acquirer_.reset( );
  }

  GlobalAddress< T > address() { return address_; }

  /// Dereference cache
  operator const T*() { 
    block_until_acquired();
    DVLOG(5) << "Const dereference of " << address_ << " * " << count_;
    return pointer_;
  } 

  /// Dereference cache
  operator const void*() { 
    block_until_acquired();
    DVLOG(5) << "Const void * dereference of " << address_ << " * " << count_;
    return pointer_;
  } 
};

/// Read-write cache object. This is parameterize so it can implement
/// coherent or incoherent read-write caches.
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
  /// Create a cache object. Call with a global address, a number of
  /// elements to fetch, and optionally a local buffer to store the
  /// cached copy.
  explicit CacheRW( GlobalAddress< T > address, size_t count, T * buffer = NULL )
    : address_( address )
    , count_( count )
    , storage_( buffer, count )
    , pointer_( storage_.pointer() )
    , acquirer_( &address_, &count_, &pointer_ )
    , releaser_( &address_, &count_, &pointer_ )
  { }

  ~CacheRW() {
    block_until_released();
  }

  /// send acquire message
  void start_acquire( ) { 
    CacheMetrics::count_rw_acquire( sizeof(T)*count_ );
    acquirer_.start_acquire( );
  }

  /// block until acquire is completed
  void block_until_acquired() {
    CacheMetrics::count_rw_acquire( sizeof(T)*count_ );
    acquirer_.block_until_acquired();
  }
  
  /// send release message
  void start_release() { 
    CacheMetrics::count_rw_release( sizeof(T)*count_ );
    releaser_.start_release( );
  }

  /// block until release is completed
  void block_until_released() {
    CacheMetrics::count_rw_release( sizeof(T)*count_ );
    releaser_.block_until_released( );
  }

  /// reassign cache to point at a different block
  void reset( GlobalAddress< T > address, size_t count ) {
    block_until_acquired();
    block_until_released();
    address_ = address;
    count_ = count;
    releaser_.reset( );
    acquirer_.reset( );
  }

  GlobalAddress< T > address() { return address_; }

  /// Dereference cache
  operator T*() { 
    block_until_acquired();
    DVLOG(5) << "RW dereference of " << address_ << " * " << count_;
    return pointer_;
  } 
  /// Dereference cache
  operator void*() { 
    block_until_acquired();
    DVLOG(5) << "RW dereference of " << address_ << " * " << count_;
    return pointer_;
  } 
};

/// Write-only cache object. This is parameterize so it can implement
/// coherent or incoherent write-only caches. This is used to do bulk
/// writes into arrays.
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
  /// Create a cache object. Call with a global address, a number of
  /// elements to fetch, and optionally a local buffer to store the
  /// cached copy.
  explicit CacheWO( GlobalAddress< T > address, size_t count, T * buffer = NULL )
  : address_( address )
  , count_( count )
  , storage_( buffer, count )
  , pointer_( storage_.pointer() )
  , acquirer_( &address_, &count_, &pointer_ )
  , releaser_( &address_, &count_, &pointer_ )
  {
    VLOG(6) << "pointer_ = " << pointer_ << ", &pointer_ = " << &pointer_ << ", storage_.pointer() = " << storage_.pointer();
  }
  
  ~CacheWO() {
    block_until_released();
  }
  
  /// send acquire message
  void start_acquire( ) {
    acquirer_.start_acquire( );
  }

  /// block until acquire is completed
  void block_until_acquired() {
    acquirer_.block_until_acquired();
  }

  /// block until release is completed
  void start_release() { 
    CacheMetrics::count_wo_release( sizeof(T)*count_ );
    releaser_.start_release( );
  }

  /// block until release is completed
  void block_until_released() {
    CacheMetrics::count_wo_release( sizeof(T)*count_ );
    releaser_.block_until_released( );
  }

  /// reassign cache to point at a different block
  /// May only take a valid address.
  void reset( GlobalAddress< T > address, size_t count ) {
    block_until_acquired();
    block_until_released();
    address_ = address;
    count_ = count;
    releaser_.reset( );
    acquirer_.reset( );
  }

  GlobalAddress< T > address() { return address_; }

  /// Dereference cache
  operator T*() { 
    block_until_acquired();
    DVLOG(5) << "WO dereference of " << address_ << " * " << count_;
    VLOG(6) << "pointer_ = " << pointer_;
    return pointer_;
  } 
  /// Dereference cache
  operator void*() { 
    block_until_acquired();
    DVLOG(5) << "WO dereference of " << address_ << " * " << count_;
    return pointer_;
  }
};

/// All the incoherent caches with the right fields filled in.
template< typename T >
struct Incoherent {
  typedef CacheRO< T, CacheAllocator, IncoherentAcquirer, NullReleaser > RO;
  typedef CacheRW< T, CacheAllocator, IncoherentAcquirer, IncoherentReleaser > RW;
  typedef CacheWO< T, CacheAllocator, NullAcquirer, IncoherentReleaser > WO;
};

/// @}

///
/// Wrapper functions for making it simpler to use the bare-bones tasking
///

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
