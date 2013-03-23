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

public:
  boost::interprocess::fixed_managed_shared_memory segment;


public:

  LocaleSharedMemory();

  // called before gasnet is ready to operate
  void init();

  // called after gasnet is ready to operate, but before user_main
  void activate();

  // clean up before shutting down
  void finish();

};

/// global LocaleSharedMemory instance
extern LocaleSharedMemory locale_shared_memory;

} // namespace impl
} // namespace Grappa

#endif
