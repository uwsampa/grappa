
#include "LocaleSharedMemory.hpp"

#ifndef SHMMAX
#error "no SHMMAX defined for this system -- look it up with the command: `sysctl -A | grep shm`"
#endif

const int64_t default_locale_reserved_size = (1L << 32);
DEFINE_int64( locale_reserved_size, default_locale_reserved_size, "Shared memory between cores on node, reserved for runtime" );
DEFINE_int64( locale_shared_size, SHMMAX + FLAGS_locale_reserved_size, "Total shared memory between cores on node" );

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
  region_size = FLAGS_locale_shared_size;
  VLOG(2) << "Creating LocaleSharedMemory region " << region_name 
          << " with " << region_size << " bytes"
          << " on " << global_communicator.mycore() 
          << " of " << global_communicator.cores();

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
    boost::interprocess::shared_memory_object::remove( region_name.c_str() );
    failure_function();
    throw;
  }
  VLOG(2) << "Created LocaleSharedMemory region " << region_name 
          << " with " << region_size << " bytes"
          << " on " << global_communicator.mycore() 
          << " of " << global_communicator.cores();
}

void LocaleSharedMemory::attach() {
  // first, check that we can create something large enough 
  CHECK_GE( FLAGS_locale_shared_size, global_bytes_per_locale ) 
    << "Can't accomodate " << global_bytes_per_locale << " bytes of shared heap per locale with " 
    << FLAGS_locale_shared_size << " total shared bytes.";


  VLOG(2) << "Attaching to LocaleSharedMemory region " << region_name 
          << " on " << global_communicator.mycore() 
          << " of " << global_communicator.cores();
  try {
    segment = boost::interprocess::fixed_managed_shared_memory( boost::interprocess::open_only,
                                                                region_name.c_str(),
                                                                base_address );
  }
  catch(...){
    boost::interprocess::shared_memory_object::remove( region_name.c_str() );
    failure_function();
    throw;
  }
  VLOG(2) << "Attached to LocaleSharedMemory region " << region_name 
          << " on " << global_communicator.mycore() 
          << " of " << global_communicator.cores();
}

void LocaleSharedMemory::destroy() {
  VLOG(2) << "Removing LocaleSharedMemory region " << region_name 
          << " on " << global_communicator.mycore() 
          << " of " << global_communicator.cores();
  boost::interprocess::shared_memory_object::remove( region_name.c_str() );
  VLOG(2) << "Removed LocaleSharedMemory region " << region_name 
          << " on " << global_communicator.mycore() 
          << " of " << global_communicator.cores();
}


LocaleSharedMemory::LocaleSharedMemory()
  : region_size( FLAGS_locale_shared_size )
  , region_name( "GrappaLocaleSharedMemory" )
  , base_address( reinterpret_cast<void*>( 0x400000000000L ) )
  , segment() // default constructor; initialize later
{ 
  boost::interprocess::shared_memory_object::remove( region_name.c_str() );

  // TODO: figure out reasonable region size
  // maybe reuse SHMMAX stuff?
  
}

LocaleSharedMemory::~LocaleSharedMemory() {
  boost::interprocess::shared_memory_object::remove( region_name.c_str() );
}

void LocaleSharedMemory::init() {
  if( FLAGS_locale_reserved_size != default_locale_reserved_size ) {
    FLAGS_locale_shared_size = SHMMAX + FLAGS_locale_reserved_size;
  }
}

void LocaleSharedMemory::activate() {
  if( Grappa::locale_mycore() == 0 ) { create(); }
  global_communicator.barrier();
  if( Grappa::locale_mycore() != 0 ) { attach(); }
  global_communicator.barrier();
}

void LocaleSharedMemory::finish() {
  global_communicator.barrier();
  if( Grappa::locale_mycore() == 0 ) { destroy(); }
}

void * LocaleSharedMemory::allocate( size_t size ) {
  void * p = NULL;
  try {
    p = Grappa::impl::locale_shared_memory.segment.allocate( size );
  }
  catch(...){
    LOG(ERROR) << "Allocation of " << size << " bytes failed with " 
               << get_free_memory() << " free.";
    failure_function();
    throw;
  }
  return p;
}

void * LocaleSharedMemory::allocate_aligned( size_t size, size_t alignment ) {
  void * p = NULL;
  try {
    p = Grappa::impl::locale_shared_memory.segment.allocate_aligned( size, alignment );
  }
  catch(...){
    LOG(ERROR) << "Allocation of " << size << " bytes with alignment " << alignment 
               << " failed with " << get_free_memory() << " free.";
    failure_function();
    throw;
  }
  return p;
}

void LocaleSharedMemory::deallocate( void * ptr ) {
  try {
    Grappa::impl::locale_shared_memory.segment.deallocate( ptr );
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
