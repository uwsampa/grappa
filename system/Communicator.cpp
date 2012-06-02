
#include <cassert>

#ifdef HEAPCHECK
#include <gperftools/heap-checker.h>
extern HeapLeakChecker * SoftXMT_heapchecker;
#endif

#include "Communicator.hpp"

// global communicator instance
Communicator global_communicator;

///construct
Communicator::Communicator( )
  : handlers_()
  , registration_is_allowed_( false )
  , communication_is_allowed_( false )
  , stats()
{ }


/// initialize communication layer. After this call, node parameters
/// may be queried and handlers may be registered, but no
/// communication is allowed.
void Communicator::init( int * argc_p, char ** argv_p[] ) {
#ifdef HEAPCHECK
  {
    HeapLeakChecker::Disabler disable_leak_checks_here;
#endif
  GASNET_CHECK( gasnet_init( argc_p, argv_p ) ); 
#ifdef HEAPCHECK
  }
#endif
  // make sure the Node type is big enough for our system
  assert( static_cast< int64_t >( gasnet_nodes() ) <= (1L << sizeof(Node) * 8) && 
          "Node type is too small for number of nodes in job" );
  registration_is_allowed_ = true;
}

#define ONE_MEGA (1024 * 1024)
#define SHARED_PROCESS_MEMORY_SIZE  (0 * ONE_MEGA)
#define SHARED_PROCESS_MEMORY_OFFSET (0 * ONE_MEGA)
/// activate communication layer. finishes registering handlers and
/// any shared memory segment. After this call, network communication
/// is allowed, but no more handler registrations are allowed.
void Communicator::activate() {
    assert( registration_is_allowed_ );
  GASNET_CHECK( gasnet_attach( &handlers_[0], handlers_.size(), // install active message handlers
                               SHARED_PROCESS_MEMORY_SIZE,
                               SHARED_PROCESS_MEMORY_OFFSET ) );
  stats.reset_clock();
  registration_is_allowed_ = false;
  communication_is_allowed_ = true;
}

/// tear down communication layer.
void Communicator::finish(int retval) {
  communication_is_allowed_ = false;
  // TODO: for now, don't call gasnet exit. should we in future?
  //gasnet_exit( retval );
}

static void comm_stats_merge_am(CommunicatorStatistics * other, size_t sz, void* payload, size_t psz) {
  global_communicator->stats.merge(other);
}

#include "SoftXMT.hpp"

static void merge_stats_task(int64_t target) {
  SoftXMT_call_on(target, &comm_stats_merge_am, &global_communicator->stats);
}

void Communicator::merge_stats() {
  Node me = SoftXMT_mynode();
  for (Node n=0; n<SoftXMT_nodes(); n++) {
    if (n != me) {
      SoftXMT_remote_privateTask(&merge_stats_task, (int64_t)me, n);
    }
  }
}

