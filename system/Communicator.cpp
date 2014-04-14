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

#include <cassert>

#include <gflags/gflags.h>

#include <boost/interprocess/shared_memory_object.hpp>

#ifdef HEAPCHECK_ENABLE
#include <gperftools/heap-checker.h>
extern HeapLeakChecker * Grappa_heapchecker;
#endif

#include "Communicator.hpp"
#include "LocaleSharedMemory.hpp"

DEFINE_int64( log2_concurrent_receives, 5, "How many receive requests do we keep active at a time?" );
DEFINE_int64( log2_concurrent_collectives, 5, "How many collective requests do we keep active at a time?" );

DEFINE_int64( log2_concurrent_sends, 5, "How many send requests do we keep active at a time?" );

DEFINE_int64( log2_buffer_size, 20, "Size of Communicator buffers" );

// // other metrics
// GRAPPA_DEFINE_METRIC( SimpleMetric<uint64_t>, communicator_messages, 0);
// GRAPPA_DEFINE_METRIC( SimpleMetric<uint64_t>, communicator_bytes, 0);
// GRAPPA_DEFINE_METRIC( CallbackMetric<double>, communicator_start_time, []() {
//     // initialization value
//     return Grappa::walltime();
//     });
// GRAPPA_DEFINE_METRIC( CallbackMetric<double>, communicator_end_time, []() {
//     // sampling value
//     return Grappa::walltime();
//     });

/// Global communicator instance
Communicator global_communicator;
  

/// Construct communicator
Communicator::Communicator( )
  : mycore_( -1 )
  , mylocale_( -1 )
  , locale_mycore_( -1 )
  , cores_( -1 )
  , locales_( -1 )
  , locale_cores_( -1 )
  , locale_of_core_()

  , receives()
  , receive_head(0)
  , receive_dispatch(0)
  , receive_tail(0)
  , receive_mask(0)
    
  , sends()
  , send_head(0)
  , send_tail(0)
  , send_mask(0)

  , barrier_request( MPI_REQUEST_NULL )
  , external_sends()
    
  , mycore( mycore_ )
  , cores( cores_ )
  , mylocale( mylocale_ )
  , locales( locales_ )
  , locale_mycore( locale_mycore_ )
  , locale_cores( locale_cores_ )
#ifdef VTRACE_FULL
  , communicator_grp_vt( VT_COUNT_GROUP_DEF( "Communicator" ) )
  , send_ev_vt( VT_COUNT_DEF( "Sends", "bytes", VT_COUNT_TYPE_UNSIGNED, communicator_grp_vt ) )
#endif
{ }


/// initialize communication layer. After this call, node parameters
/// may be queried and handlers may be registered, but no
/// communication is allowed.
void Communicator::init( int * argc_p, char ** argv_p[] ) {
#ifdef HEAPCHECK_ENABLE
  {
    HeapLeakChecker::Disabler disable_leak_checks_here;
#endif
  MPI_CHECK( MPI_Init( argc_p, argv_p ) ); 
#ifdef HEAPCHECK_ENABLE
  }
#endif

  // initialize masks
  receive_mask = (1 << FLAGS_log2_concurrent_receives) - 1;
  send_mask = (1 << FLAGS_log2_concurrent_sends) - 1;
  
  // initialize job geometry
  int mycoreint, coresint;
  MPI_CHECK( MPI_Comm_rank( MPI_COMM_WORLD, &mycoreint ) );
  MPI_CHECK( MPI_Comm_size( MPI_COMM_WORLD, &coresint ) );
  mycore_ = mycoreint;
  cores_ = coresint;
  
  // try to compute locale geometry
  char * locale_rank_string = NULL;

  // are we using Slurm?
  if( locale_rank_string = getenv("SLURM_LOCALID") ) {
    locale_mycore_ = atoi(locale_rank_string);

    char * locale_size_string = getenv("SLURM_TASKS_PER_NODE");
    CHECK_NOTNULL( locale_size_string );
    // TODO: verify that locale dimensions are the same for all nodes in job
    locale_cores_ = atoi(locale_size_string);

    // are we using OpenMPI?
  } else if( locale_rank_string = getenv("OMPI_COMM_WORLD_LOCAL_RANK") ) {
    locale_mycore_ = atoi(locale_rank_string);

    char * locale_size_string = getenv("OMPI_COMM_WORLD_LOCAL_SIZE");
    CHECK_NOTNULL( locale_size_string );
    locale_cores_ = atoi(locale_size_string);

    // are we using MVAPICH2?
  } else if( locale_rank_string = getenv("MV2_COMM_WORLD_LOCAL_RANK") ){
    locale_mycore_ = atoi(locale_rank_string);

    char * locale_size_string = getenv("MV2_COMM_WORLD_LOCAL_SIZE");
    CHECK_NOTNULL( locale_size_string );
    locale_cores_ = atoi(locale_size_string);

  } else {
    LOG(ERROR) << "Could not determine locale dimensions!";
    exit(1);
  }

  // verify that job has the same number of processes on each node
  int64_t my_locale_cores = locale_cores_;
  int64_t max_locale_cores = 0;
  MPI_CHECK( MPI_Allreduce( &my_locale_cores, &max_locale_cores, 1,
                            MPI_INT64_T, MPI_MAX, MPI_COMM_WORLD ) );
  CHECK_EQ( my_locale_cores, max_locale_cores )
    << "Number of processes per locale is not the same across job!";


  // Guess at rank-to-locale mapping
  // TODO: verify ranks aren't allocated in a cyclic fashion
  mylocale_ = mycore_ / locale_cores_;

  
  // Guess at number of locales
  // TODO: verify ranks aren't allocated in a cyclic fashion
  locales_ = cores_ / locale_cores_;

  // allocate and initialize core-to-locale translation
  locale_of_core_.reset( new Locale[ cores_ ] );
  for( int i = 0; i < cores_; ++i ) {
    // TODO: verify ranks aren't allocated in a cyclic fashion
    locale_of_core_[i] = i / locale_cores_;
  }

  DVLOG(2) << "hostname " << hostname()
           << " mycore_ " << mycore_ 
           << " cores_ " << cores_
           << " mylocale_ " << mylocale_ 
           << " locales_ " << locales_ 
           << " locale_mycore_ " << locale_mycore_ 
           << " locale_cores_ " << locale_cores_
           << " pid " << getpid();


  MPI_CHECK( MPI_Barrier( MPI_COMM_WORLD ) );

  //receives.reset( new Context[ 1 << FLAGS_log2_concurrent_receives ] );
  receives = new Context[ 1 << FLAGS_log2_concurrent_receives ];

  //sends.reset( new Context[ 1 << FLAGS_log2_concurrent_sends ] );
  sends = new Context[ 1 << FLAGS_log2_concurrent_sends ];
  
  MPI_CHECK( MPI_Barrier( MPI_COMM_WORLD ) );
}

                             
void Communicator::activate() {

  for( int i = 0; i < (1 << FLAGS_log2_concurrent_sends); ++i ) {
    char * buf;
    buf = (char*) Grappa::impl::locale_shared_memory.allocate_aligned( (1 << FLAGS_log2_buffer_size), 8 );
    //MPI_Alloc_mem( (1 << FLAGS_log2_buffer_size) , MPI_INFO_NULL, &buf );
    sends[i].buf = buf;
    sends[i].size = 1 << FLAGS_log2_buffer_size;
  }

  for( int i = 0; i < (1 << FLAGS_log2_concurrent_receives); ++i ) {
    char * buf;
    buf = (char*) Grappa::impl::locale_shared_memory.allocate_aligned( (1 << FLAGS_log2_buffer_size), 8 );
    //MPI_Alloc_mem( (1 << FLAGS_log2_buffer_size), MPI_INFO_NULL, &buf );
    receives[i].buf = buf;
    receives[i].size = 1 << FLAGS_log2_buffer_size;
  }

  repost_receive_buffers();

  DVLOG(3) << "Entering activation barrier";
  MPI_CHECK( MPI_Barrier( MPI_COMM_WORLD ) );
  DVLOG(3) << "Leaving activation barrier";
}


const char * Communicator::hostname() {
  static char name[ MPI_MAX_PROCESSOR_NAME ] = {0};
  static int name_size = 0;
  if( '\0' == name[0] ) {
    MPI_CHECK( MPI_Get_processor_name( &name[0], &name_size ) );
  }
  return &name[0];
}





Context * Communicator::try_get_send_context() {
  Context * c = NULL;

  if( ((send_head + 1) & send_mask) != send_tail ) {
    c = &sends[send_head];
    CHECK_EQ( c->reference_count, 0 ) << "Send context already in use!";
    send_head = (send_head + 1) & send_mask;
  }

  return c;
}

void Communicator::post_send( Context * c,
                              int dest,
                              size_t size,
                              int tag ) {
  DCHECK_GE( dest, 0 );
  c->reference_count = 1; // mark as active
  DVLOG(6) << "Posting send " << c << " to " << dest
           << " with buf " << c->buf
           << " callback " << (void*)c->callback;
  MPI_CHECK( MPI_Isend( c->buf, size, MPI_BYTE, dest, tag, MPI_COMM_WORLD, &c->request ) );
}

void Communicator::post_external_send( Context * c,
                                       int dest,
                                       size_t size,
                                       int tag ) {
  DVLOG(6) << "Posting external send " << c;
  post_send( c, dest, size, tag );
  external_sends.push_back(c);
}

void Communicator::post_receive( Context * c ) {
  MPI_CHECK( MPI_Irecv( c->buf, c->size, MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &c->request ) );
  DVLOG(6) << "Posted receive " << c << " with buf " << c->buf << " callback " << (void*) c->callback;
}




void Communicator::garbage_collect() {
  int flag;
  MPI_Status status;

  // check for completed sends and re-enable
  while( send_tail != send_head ) {
    auto c = &sends[send_tail];
    if( c->reference_count > 0 ) {
      MPI_CHECK( MPI_Test( &c->request, &flag, &status ) );
      if( flag ) {
        if( c->callback ) {
          (c->callback)( c, status.MPI_SOURCE, status.MPI_TAG, c->size );
        }
        c->reference_count = 0;
        send_tail = (send_tail + 1) & send_mask;
      }
    } else {
      break;
    }
  }

  // check external send contexts too
  while( !external_sends.empty() ) {
    auto c = external_sends.front();
    if( c->reference_count > 0 ) {
      MPI_CHECK( MPI_Test( &c->request, &flag, &status ) );
      if( flag ) {
        c->reference_count = 0;
        if( c->callback ) {
          (c->callback)( c, status.MPI_SOURCE, status.MPI_TAG, c->size );
        }
        external_sends.pop_front();
      } else {
        break;
      }
    } else {
      break;
    }
  }
  
}

void Communicator::repost_receive_buffers() {

  // consume buffers that are completely delivered
  while( receive_tail != receive_dispatch ) {
    auto c = &receives[receive_tail];
    if( 0 == c->reference_count ) {
      receive_tail = (receive_tail + 1) & receive_mask;
    } else {
      break;
    }
  }

  // repost free buffers
  while( ((receive_head + 1) & receive_mask) != receive_tail ) {
    auto c = &receives[receive_head];
    post_receive( c );
    receive_head = (receive_head + 1) & receive_mask;
  }
  
}


static void receive_buffer( Context * c, int size ) {
  auto fp = reinterpret_cast< Grappa::impl::Deserializer * >( c->buf );
  DVLOG(6) << "Calling deserializer " << (void*) (*fp) << " for " << c;
  (*fp)( (char*) (fp+1), size, c );
}

static void receive( Context * c, int size ) {
  DVLOG(6) << "Receiving " << c;
  c->reference_count = 1;
  receive_buffer( c, size );
}

void Communicator::process_received_buffers() {
  MPI_Status status;

  while( receive_dispatch != receive_head ) {
    int flag;
    auto c = &receives[receive_dispatch];
    
    MPI_CHECK( MPI_Test( &c->request, &flag, &status ) );
    
    // if message has been received
    if( flag ) {
      int size = 0;
      MPI_CHECK( MPI_Get_count( &status, MPI_BYTE, &size ) );
      // start delivering received buffer
      receive( c, size );
      if( c->callback ) {
        (c->callback)( c, status.MPI_SOURCE, status.MPI_TAG, size );
      }
      receive_dispatch = (receive_dispatch + 1) & receive_mask;
      // update if anything has finished delivery
      repost_receive_buffers();
    } else {
      break;
    }
  }
  
  // see if anything else finished delivery while we were busy
  repost_receive_buffers();
  garbage_collect();
}

void Communicator::poll( unsigned int max_receives ) {
  process_received_buffers();
}





static void finalize_mpi(void)
{
  int already_finalised;

   MPI_Finalized(&already_finalised);
   if (!already_finalised) {
     MPI_Finalize();
   }
}

/// tear down communication layer.
void Communicator::finish(int retval) {
  
  MPI_CHECK( MPI_Barrier( MPI_COMM_WORLD ) );
  
  // get rid of any outstanding sends
  for( int i = 0; i < (1 << FLAGS_log2_concurrent_sends); ++i ) {
    if( NULL != sends[i].callback ) {
      sends[i].callback = NULL;
      MPI_CHECK( MPI_Cancel( &sends[i].request ) );
    }
  }
  
  MPI_CHECK( MPI_Barrier( MPI_COMM_WORLD ) );
  
  // get rid of any outstanding receives
  for( int i = 0; i < (1 << FLAGS_log2_concurrent_receives); ++i ) {
    if( NULL != receives[i].callback ) {
      receives[i].callback = NULL;
      MPI_CHECK( MPI_Cancel( &receives[i].request ) );
    }
  }
  
  MPI_CHECK( MPI_Barrier( MPI_COMM_WORLD ) );
  
  // For VampirTrace's sake, clean up MPI only when process exits.
  atexit(finalize_mpi);
}
