#pragma once

// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

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

  void create();
  void attach();
  void destroy();

  friend class RDMAAggregator;
  friend class coro;

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

  size_t get_free_memory() const { return segment.get_free_memory(); }
  size_t get_size() const { return segment.get_size(); }
};


/// global LocaleSharedMemory instance
extern LocaleSharedMemory locale_shared_memory;

} // namespace impl

template<typename T>
inline T* locale_alloc(size_t n) {
	return reinterpret_cast<T*>(impl::locale_shared_memory.allocate(sizeof(T)*n));
}

inline void locale_free(void * ptr) {
	impl::locale_shared_memory.deallocate(ptr);
}

} // namespace Grappa

#endif
