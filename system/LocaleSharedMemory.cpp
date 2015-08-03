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

#include "LocaleSharedMemory.hpp"

DEFINE_int64( locale_shared_size, 0, "Total shared memory between cores on node (when 0, defaults to locale_shared_fraction * total node memory)" );

DEFINE_double( locale_shared_fraction, 0.5, "Fraction of total node memory to allocate for Grappa" );

DEFINE_double( locale_user_heap_fraction, 0.1, "Fraction of locale shared memory to set aside for the user" );

DEFINE_double( global_heap_fraction, 0.25, "Fraction of locale shared memory to set aside for global shared heap" );

DECLARE_int64( node_memsize );
DECLARE_bool( global_memory_use_hugepages );

// forward declarations
namespace Grappa {
namespace impl {

/// called on failures to backtrace and pause for debugger
extern void failure_function();

/// how much memory do we expect to allocate?
extern int64_t global_memory_size_bytes;
extern int64_t global_bytes_per_core;
extern int64_t global_bytes_per_locale;

}
}




namespace Grappa {
namespace impl {

/// global LocaleSharedMemory instance
LocaleSharedMemory locale_shared_memory;






void LocaleSharedMemory::create() {
  VLOG(2) << "Creating LocaleSharedMemory region " << region_name 
          << " with " << region_size << " bytes"
          << " on " << global_communicator.mycore 
          << " of " << global_communicator.cores;

  // if possible, delete this user's old leftover share memory regions
  // if this returns false, either there was nothing to delete or
  // there was something owned by somebody else.
  bool deleted_one = boost::interprocess::shared_memory_object::remove( region_name.c_str() );
  if( deleted_one ) {
    VLOG(1) << "Deleted an old, leftover " << region_name.c_str();
  }

  // now, try to allocate a new one
  try {
    segment = boost::interprocess::fixed_managed_shared_memory( boost::interprocess::create_only,
                                                                region_name.c_str(),
                                                                region_size,
                                                                base_address);
  }
  catch(...){
    LOG(ERROR) << "Failed to create locale shared memory of size " << region_size;
    boost::interprocess::shared_memory_object::remove( region_name.c_str() );
    failure_function();
    throw;
  }
  VLOG(2) << "Created LocaleSharedMemory region " << region_name 
          << " with " << region_size << " bytes"
          << " on " << global_communicator.mycore 
          << " of " << global_communicator.cores;
}

void LocaleSharedMemory::attach() {
  // first, check that we can create something large enough 
  CHECK_GE( FLAGS_locale_shared_size, global_bytes_per_locale ) 
    << "Can't accomodate " << global_bytes_per_locale << " bytes of shared heap per locale with " 
    << FLAGS_locale_shared_size << " total shared bytes.";


  VLOG(2) << "Attaching to LocaleSharedMemory region " << region_name 
          << " on " << global_communicator.mycore 
          << " of " << global_communicator.cores;
  try {
    segment = boost::interprocess::fixed_managed_shared_memory( boost::interprocess::open_only,
                                                                region_name.c_str(),
                                                                base_address );
  }
  catch(...){
    LOG(ERROR) << "Failed to attach to locale shared memory of size " << region_size;
    boost::interprocess::shared_memory_object::remove( region_name.c_str() );
    failure_function();
    throw;
  }
  VLOG(2) << "Attached to LocaleSharedMemory region " << region_name 
          << " on " << global_communicator.mycore 
          << " of " << global_communicator.cores;
}

void LocaleSharedMemory::unlink() {
  VLOG(2) << "Removing LocaleSharedMemory region " << region_name 
          << " on " << global_communicator.mycore 
          << " of " << global_communicator.cores;
  // according to docs, this just does unlink(), so object will be removed once no process is using it.
  bool success = boost::interprocess::shared_memory_object::remove( region_name.c_str() );
  if( !success ) {
    LOG(WARNING) << "Remove/unlink call filed for shared memory object " << region_name.c_str() << ".";
  }
  VLOG(2) << "Removed LocaleSharedMemory region " << region_name 
          << " on " << global_communicator.mycore 
          << " of " << global_communicator.cores;
}


LocaleSharedMemory::LocaleSharedMemory()
  : region_size()
  , region_name( "GrappaLocaleSharedMemory" )
  , base_address( reinterpret_cast<void*>( 0x400000000000L ) )
  , segment() // default constructor; initialize later
  , allocated(0)
{ 
  boost::interprocess::shared_memory_object::remove( region_name.c_str() );

  // TODO: figure out reasonable region size
  // maybe reuse total memory measurement?
  
}

LocaleSharedMemory::~LocaleSharedMemory() {
  boost::interprocess::shared_memory_object::remove( region_name.c_str() );
}

void LocaleSharedMemory::init() {
  if( 0 == FLAGS_locale_shared_size ) {
    double locale_shared_size = FLAGS_locale_shared_fraction * static_cast< double >( FLAGS_node_memsize );
    region_size = static_cast< int64_t >( locale_shared_size );
    FLAGS_locale_shared_size = region_size;
  }
}

void LocaleSharedMemory::activate() {
  if( Grappa::locale_mycore() == 0 ) { create(); }
  global_communicator.barrier();
  if( Grappa::locale_mycore() != 0 ) { attach(); }
  global_communicator.barrier();
  if( Grappa::locale_mycore() == 0 ) { unlink(); } // delete once everyone has released it
  //available = global_bytes_per_core;
}

void LocaleSharedMemory::finish() {
  // let Boost's atexit() handler take care of this
  //global_communicator.barrier(); // we should have a barrier before destroying the shared memory region
  //if( Grappa::locale_mycore() == 0 ) { destroy(); }
}

void * LocaleSharedMemory::allocate( size_t size ) {
  void * p = NULL;
  try {
    p = segment.allocate( size );
    allocated += size;
  }
  catch(...){
    LOG(ERROR) << "Allocation of " << size << " bytes failed with " 
               << get_free_memory() << " free and "
               << allocated << " allocated locally";
    failure_function();
    throw;
  }
  return p;
}

void * LocaleSharedMemory::allocate_aligned( size_t size, size_t alignment ) {
  void * p = NULL;
  try {
    p = segment.allocate_aligned( size, alignment );
    allocated += size;
  }
  catch(...){
    LOG(ERROR) << "Allocation of " << size << " bytes with alignment " << alignment 
               << " failed with " << get_free_memory() << " free and "
               << allocated << " allocated locally";
    failure_function();
    throw;
  }
  return p;
}

void LocaleSharedMemory::deallocate( void * ptr ) {
  try {
    segment.deallocate( ptr );
  }
  catch(...){
    LOG(ERROR) << "Deallocation of " << ptr
               << " failed with " << get_free_memory() << " free.";
    failure_function();
    throw;
  }
}



} // namespace impl
} // namespace Grappa
