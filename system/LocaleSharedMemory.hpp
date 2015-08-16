////////////////////////////////////////////////////////////////////////
// Copyright (c) 2010-2015, University of Washington and Battelle
// Memorial Institute.  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//     * Redistributions of source code must retain the above
//       copyright notice, this list of conditions and the following
//       disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials
//       provided with the distribution.
//     * Neither the name of the University of Washington, Battelle
//       Memorial Institute, or the names of their contributors may be
//       used to endorse or promote products derived from this
//       software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// UNIVERSITY OF WASHINGTON OR BATTELLE MEMORIAL INSTITUTE BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
////////////////////////////////////////////////////////////////////////
#pragma once

#ifndef __LOCALE_SHARED_MEMORY_HPP__
#define __LOCALE_SHARED_MEMORY_HPP__

#include <gflags/gflags.h>
#include <glog/logging.h>

#include <string>

#include <boost/interprocess/managed_shared_memory.hpp>

#include "Communicator.hpp"

namespace Grappa {
namespace impl {

class LocaleSharedMemory {
private:
  size_t region_size;
  std::string region_name;
  void * base_address;
  
  size_t allocated;

  void create();
  void attach();
  void unlink();

  friend class RDMAAggregator;

public: // TODO: fix Gups
  boost::interprocess::fixed_managed_shared_memory segment;

public:

  LocaleSharedMemory();
  ~LocaleSharedMemory();

  // called before gasnet is ready to operate
  void init();

  // called after gasnet is ready to operate, but before user_main
  void activate();

  // clean up before shutting down
  void finish();

  // make sure an address is in the locale shared memory
  inline void validate_address( void * addr ) {
    //#ifndef NDEBUG
    char * char_base = reinterpret_cast< char* >( base_address );
    char * char_addr = reinterpret_cast< char* >( addr );
    CHECK( (char_base <= addr) && (addr < (char_base + region_size) ) )
      << "Address " << addr << "  out of locale shared range!";
  }
    //#endif

  void * allocate( size_t size );
  void * allocate_aligned( size_t size, size_t alignment );
  void deallocate( void * ptr );

  const size_t get_free_memory() const { return segment.get_free_memory(); }
  const size_t get_size() const { return segment.get_size(); }
  const size_t get_allocated() const { return allocated; }
};


/// global LocaleSharedMemory instance
extern LocaleSharedMemory locale_shared_memory;

} // namespace impl

/// @addtogroup Memory
/// @{

/// Allocate memory in locale shared heap.
template<typename T>
inline T* locale_alloc(size_t n = 1) {
  return reinterpret_cast<T*>(impl::locale_shared_memory.allocate(sizeof(T)*n));
}

inline void* locale_alloc(size_t n = 1) {
  return impl::locale_shared_memory.allocate(n);
}

template<typename T>
inline T* locale_alloc_aligned(size_t alignment, size_t n = 1) {
  return reinterpret_cast<T*>(impl::locale_shared_memory.allocate_aligned(sizeof(T)*n, alignment));
}

inline void* locale_alloc_aligned(size_t alignment, size_t n = 1) {
  return impl::locale_shared_memory.allocate_aligned(n, alignment);
}

/// allocate an object in the locale shared heap, passing arguments to its constructor
template< typename T, typename... Args >
inline T* locale_new(Args&&... args) {
  return new (locale_alloc<T>()) T(std::forward<Args...>(args...));
}

/// allocate an object in the locale shared heap
template< typename T >
inline T* locale_new() {
  return new (locale_alloc<T>()) T();
}

/// allocate an array in the locale shared heap
template< typename T >
inline T* locale_new_array(size_t n = 1) {
  return new (locale_alloc<T>(n)) T[n];
}

/// Free memory that was allocated from locale shared heap.
inline void locale_free(void * ptr) {
  impl::locale_shared_memory.deallocate(ptr);
}

/// @}
} // namespace Grappa

#endif
