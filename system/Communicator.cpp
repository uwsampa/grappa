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

#include <mpi.h>

#ifdef HEAPCHECK_ENABLE
#include <gperftools/heap-checker.h>
extern HeapLeakChecker * Grappa_heapchecker;
#endif

#include "Communicator.hpp"

DEFINE_int64( log2_concurrent_receives, 4, "How many receive requests do we keep active at a time?" );
DEFINE_int64( log2_concurrent_collectives, 4, "How many collective requests do we keep active at a time?" );

DEFINE_int64( log2_concurrent_sends, 4, "How many send requests do we keep active at a time?" );

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
  , receive_tail(0)
  , sends()
  , available_sends()
  , barrier_request( MPI_REQUEST_NULL )
    
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

  // initialize job geometry
  int mycoreint, coresint;
  MPI_CHECK( MPI_Comm_rank( MPI_COMM_WORLD, &mycoreint ) );
  MPI_CHECK( MPI_Comm_size( MPI_COMM_WORLD, &coresint ) );
  mycore_ = mycoreint;
  cores_ = coresint;
  LOG(INFO) << "Mycore " << mycore_ << " cores " << cores_;
  
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

  DVLOG(2) << " mycore_ " << mycore_ 
           << " cores_ " << cores_
           << " mylocale_ " << mylocale_ 
           << " locales_ " << locales_ 
           << " locale_mycore_ " << locale_mycore_ 
           << " locale_cores_ " << locale_cores_
           << " pid " << getpid();


  MPI_CHECK( MPI_Barrier( MPI_COMM_WORLD ) );

  receives.reset( new Context[ 1 << FLAGS_log2_concurrent_receives ] );

  sends.reset( new Context[ 1 << FLAGS_log2_concurrent_sends ] );
  for( int i = 0; i < (1 << FLAGS_log2_concurrent_sends); ++i ) {
    available_sends.push( &sends[i] ); 
  }
  
  MPI_CHECK( MPI_Barrier( MPI_COMM_WORLD ) );
}



void Communicator::activate() {
  // nothing!
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





Context * Communicator::get_context() {
  // first try
  if( available_sends.empty() ) {
    garbage_collect();
  }

  // second try: block
  while( available_sends.empty() ) {
    poll(128);
    garbage_collect();
  }

  CHECK( !available_sends.empty() );

  Context * c = available_sends.top();
  available_sends.pop();

  return c;
}


void Communicator::post_send( int dest,
                              void * buf, size_t size,
                              void (*callback)(int source, int tag, void * buf, size_t size ),
                              int tag ) {
  auto c = get_context();
  c->buf = buf;
  c->callback = callback;
  MPI_CHECK( MPI_Isend( buf, size, MPI_BYTE, dest, tag, MPI_COMM_WORLD, &c->request ) );
  
}

void Communicator::post_receive( void * buf, size_t size,
                                 void (*callback)(int source, int tag, void * buf, size_t size ),
                                 int tag, int source ) {
  Context * c = &receives[receive_head];
  CHECK_NULL( c->callback );

  int receive_mask = (1 << FLAGS_log2_concurrent_receives) - 1;
  receive_head = (receive_head + 1) & receive_mask;

  c->callback = callback;
  c->buf = buf;
  MPI_CHECK( MPI_Irecv( buf, size, MPI_BYTE, source, tag, MPI_COMM_WORLD, &c->request ) );
}




void Communicator::garbage_collect() {
  // check for completed sends and re-enable
  for( int i = 0; i < (1 << FLAGS_log2_concurrent_sends); ++i ) {
    if( NULL != sends[i].callback ) {
      int flag;
      MPI_CHECK( MPI_Test( &sends[i].request, &flag, MPI_STATUS_IGNORE ) );
      if( flag ) {
        sends[i].callback = NULL;
        available_sends.push( &sends[i] );
      }
    }
  }
}


void Communicator::poll( int max_receives ) {
  for( int i = 0; i < max_receives && receives[receive_tail].callback != NULL; ++i ) {
    int flag;
    MPI_Status status;
    
    DVLOG(5) << "Testing message " << &receives[receive_tail];
    MPI_CHECK( MPI_Test( &receives[receive_tail].request, &flag, &status ) );
    
    // if message has been received
    if( flag ) {
      Context * c = &receives[receive_tail];
      int receive_mask = (1 << FLAGS_log2_concurrent_receives) - 1;
      receive_tail = (receive_tail + 1) & receive_mask;
      
      auto callback = c->callback;
      void * buf = c->buf;
      
      // disable message
      c->callback = NULL;
      c->buf = NULL;
      
      int size;
      MPI_CHECK( MPI_Get_count( &status, MPI_BYTE, &size ) );
      // call the local callback bound to this receive request to process the received buffer.
      // we assume the callback already has a pointer to the buffer and its size.
      DVLOG(5) << "Receiving message " << c;
      callback( status.MPI_SOURCE, status.MPI_TAG, buf, size );
      
      // move to next message
    } else {
      break;
    }
  }
  //garbage_collect();
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
