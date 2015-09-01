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

#include <cassert>
#include <limits>

#include <gflags/gflags.h>

#include <boost/interprocess/shared_memory_object.hpp>

#ifdef HEAPCHECK_ENABLE
#include <gperftools/heap-checker.h>
extern HeapLeakChecker * Grappa_heapchecker;
#endif

#include "Communicator.hpp"
#include "LocaleSharedMemory.hpp"

#ifndef COMMUNICATOR_TEST
#include "Metrics.hpp"
#endif

DEFINE_int64( log2_concurrent_receives, 7, "How many receive requests do we keep active at a time?" );
DEFINE_int64( log2_concurrent_sends, 7, "How many send requests do we keep active at a time?" );

static const int MIN_CONCURRENT_BUFFERS = 2;

/// Size of communication buffers (2^log2_buffer_size)
DEFINE_int64( log2_buffer_size, 19, "Size of Communicator buffers" );

static const int MIN_LOG2_BUFFER_SIZE = 15;

#ifndef COMMUNICATOR_TEST
// // other metrics
// GRAPPA_DEFINE_METRIC( SimpleMetric<uint64_t>, communicator_messages, 0);
// GRAPPA_DEFINE_METRIC( SimpleMetric<uint64_t>, communicator_bytes, 0);

GRAPPA_DEFINE_METRIC( SummarizingMetric<int64_t>, communicator_message_bytes, 0 );

// GRAPPA_DEFINE_METRIC( CallbackMetric<double>, communicator_start_time, []() {
//     // initialization value
//     return Grappa::walltime();
//     });
// GRAPPA_DEFINE_METRIC( CallbackMetric<double>, communicator_end_time, []() {
//     // sampling value
//     return Grappa::walltime();
//     });
#endif

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
  , collective_context(NULL)

  , locale_comm()
  , grappa_comm()
    
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


/// Initialize communication layer. This method uses MPI_COMM_WORLD
/// for job setup, but then all communication happens over a new
/// communicator.
void Communicator::init( int * argc_p, char ** argv_p[] ) {
#ifdef HEAPCHECK_ENABLE
  {
    HeapLeakChecker::Disabler disable_leak_checks_here;
#endif
  MPI_CHECK( MPI_Init( argc_p, argv_p ) ); 
#ifdef HEAPCHECK_ENABLE
  }
#endif


  // get locale-local communicator
  MPI_CHECK( MPI_Comm_split_type( MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, 0, MPI_INFO_NULL, &locale_comm ) );
  int locale_mycoreint = -1;
  int locale_coresint = -1;
  MPI_CHECK( MPI_Comm_rank( locale_comm, &locale_mycoreint ) );
  MPI_CHECK( MPI_Comm_size( locale_comm, &locale_coresint ) );
  locale_mycore_ = locale_mycoreint;
  locale_cores_ = locale_coresint;
  
  // get locale count
  int32_t localesint = locale_mycoreint == 0; // count one per locale and broadcast
  MPI_CHECK( MPI_Allreduce( MPI_IN_PLACE, &localesint, 1, MPI_INT32_T,
                            MPI_SUM, MPI_COMM_WORLD ) );
  locales_ = localesint;
  
  // get my locale
  int32_t mylocaleint = locale_mycoreint == 0;  // count one per locale and sum
  MPI_CHECK( MPI_Scan( MPI_IN_PLACE, &mylocaleint, 1, MPI_INT32_T,
                       MPI_SUM, MPI_COMM_WORLD ) );
  // copy to all cores in locale
  MPI_CHECK( MPI_Bcast( &mylocaleint, 1, MPI_INT32_T,
                        0, locale_comm ) );
  mylocaleint -= 1; // make zero-indexed
  mylocale_ = mylocaleint;
  
  // make new communicator with ranks laid out like we want
  MPI_CHECK( MPI_Comm_split( MPI_COMM_WORLD, 0, mylocaleint, &grappa_comm ) );
  int grappa_mycoreint = -1;
  int grappa_coresint = -1;
  MPI_CHECK( MPI_Comm_rank( grappa_comm, &grappa_mycoreint ) );
  MPI_CHECK( MPI_Comm_size( grappa_comm, &grappa_coresint ) );
  mycore_ = grappa_mycoreint;
  cores_ = grappa_coresint;
    
  // allocate and initialize core-to-locale translation
  locale_of_core_.reset( new Locale[ cores_ ] );
  MPI_CHECK( MPI_Allgather( &mylocale_, 1, MPI_INT16_T,
                            &locale_of_core_[0], 1, MPI_INT16_T,
                            grappa_comm ) );

  
  // verify locale numbering is consistent with locales
  int32_t localemin = std::numeric_limits<int32_t>::max();
  int32_t localemax = std::numeric_limits<int32_t>::min();
  MPI_CHECK( MPI_Reduce( &mylocaleint, &localemin, 1, MPI_INT32_T,
                         MPI_MIN, 0, locale_comm ) );
  MPI_CHECK( MPI_Reduce( &mylocaleint, &localemax, 1, MPI_INT32_T,
                         MPI_MAX, 0, locale_comm ) );
  if( 0 == locale_mycoreint ) {
    CHECK_EQ( localemin, localemax ) << "Locale ID is not consistent across locale!";
  }

  // verify locale core count is the same across job
  int32_t locale_coresmin = std::numeric_limits<int32_t>::max();
  int32_t locale_coresmax = std::numeric_limits<int32_t>::min();
  MPI_CHECK( MPI_Reduce( &locale_coresint, &locale_coresmin, 1, MPI_INT32_T,
                         MPI_MIN, 0, grappa_comm ) );
  MPI_CHECK( MPI_Reduce( &locale_coresint, &locale_coresmax, 1, MPI_INT32_T,
                         MPI_MAX, 0, grappa_comm ) );
  if( 0 == grappa_mycoreint ) {
    CHECK_EQ( locale_coresmin, locale_coresmax ) << "Number of cores per locale is not the same across job!";
  }

  
  DVLOG(2) << "hostname " << hostname()
           << " mycore_ " << mycore_ 
           << " cores_ " << cores_
           << " mylocale_ " << mylocale_ 
           << " locales_ " << locales_ 
           << " locale_mycore_ " << locale_mycore_ 
           << " locale_cores_ " << locale_cores_
           << " pid " << getpid();

  // this will let our error wrapper actually fire.
  MPI_CHECK( MPI_Comm_set_errhandler( locale_comm, MPI_ERRORS_RETURN ) );
  MPI_CHECK( MPI_Comm_set_errhandler( grappa_comm, MPI_ERRORS_RETURN ) );


  MPI_CHECK( MPI_Barrier( grappa_comm ) );

  // initialize masks
  receive_mask = (1 << FLAGS_log2_concurrent_receives) - 1;
  send_mask = (1 << FLAGS_log2_concurrent_sends) - 1;
  
  receives = new CommunicatorContext[ 1 << FLAGS_log2_concurrent_receives ];

  sends = new CommunicatorContext[ 1 << FLAGS_log2_concurrent_sends ];
  
  MPI_CHECK( MPI_Barrier( grappa_comm ) );
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
  MPI_CHECK( MPI_Barrier( grappa_comm ) );
  DVLOG(3) << "Leaving activation barrier";
}

size_t Communicator::estimate_footprint() const {
  auto buf_size = (1L << FLAGS_log2_buffer_size);
  auto n = (1L << FLAGS_log2_concurrent_sends) + (1L << FLAGS_log2_concurrent_receives);
  return buf_size * n;
}

size_t Communicator::adjust_footprint(size_t target) {
  
  auto& sends = FLAGS_log2_concurrent_sends;
  auto& recvs = FLAGS_log2_concurrent_receives;
  auto& size  =  FLAGS_log2_buffer_size;
  
  if (estimate_footprint() > target) {
    if (mycore == 0) LOG(WARNING) << "Adjusting to fit in target footprint: " << target << " bytes";
    while (estimate_footprint() > target) {
      // first try adjusting number of concurrent buffers
      if (sends > MIN_CONCURRENT_BUFFERS && sends >= recvs) sends--;
      else if (recvs > MIN_CONCURRENT_BUFFERS && recvs >= sends) recvs--;
      // next try making buffers smaller
      else if (size > MIN_LOG2_BUFFER_SIZE) size--;
      // otherwise we just have to return what we've got (we're at the minimum allowable footprint)
      else break;
    }
  }
  
  if (mycore == 0) VLOG(2) << "\nAdjusted:"
    << "\n  estimated footprint:      " << estimate_footprint()
    << "\n  log2_concurrent_sends:    " << sends
    << "\n  log2_concurrent_receives: " << recvs
    << "\n  log2_buffer_size:         " << size;
  return estimate_footprint();
}


const char * Communicator::hostname() {
  static char name[ MPI_MAX_PROCESSOR_NAME ] = {0};
  static int name_size = 0;
  if( '\0' == name[0] ) {
    MPI_CHECK( MPI_Get_processor_name( &name[0], &name_size ) );
  }
  return &name[0];
}





CommunicatorContext * Communicator::try_get_send_context() {
  CommunicatorContext * c = NULL;

  if( ((send_head + 1) & send_mask) != send_tail ) {
    c = &sends[send_head];
    CHECK_EQ( c->reference_count, 0 ) << "Send context already in use!";
    send_head = (send_head + 1) & send_mask;
  }

  return c;
}

void Communicator::post_send( CommunicatorContext * c,
                              int dest,
                              size_t size,
                              int tag ) {
  DCHECK_GE( dest, 0 );
  c->reference_count = 1; // mark as active
  DVLOG(6) << "Posting send " << c << " to " << dest
           << " with buf " << c->buf
           << " callback " << (void*)c->callback;
  MPI_CHECK( MPI_Isend( c->buf, size, MPI_BYTE, dest, tag, grappa_comm, &c->request ) );
#ifndef COMMUNICATOR_TEST
  communicator_message_bytes += size;
#endif
}

void Communicator::post_external_send( CommunicatorContext * c,
                                       int dest,
                                       size_t size,
                                       int tag ) {
  DVLOG(6) << "Posting external send " << c;
  post_send( c, dest, size, tag );
  external_sends.push_back(c);
#ifndef COMMUNICATOR_TEST
  communicator_message_bytes += size;
#endif
}

void Communicator::post_receive( CommunicatorContext * c ) {
  MPI_CHECK( MPI_Irecv( c->buf, c->size, MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG, grappa_comm, &c->request ) );
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
      } else { // not sent yet
        break;
      }
    } else { // not reference count > 0
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
      } else { // not sent yet
        break;
      }
    } else { // not reference_count > 0
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


static void receive_buffer( CommunicatorContext * c, int size ) {
  auto fp = reinterpret_cast< Grappa::impl::Deserializer * >( c->buf );
  DVLOG(6) << "Calling deserializer " << (void*) (*fp) << " for " << c;
  (*fp)( (char*) (fp+1), (size - sizeof(Grappa::impl::Deserializer)), c );
}

static void receive( CommunicatorContext * c, int size ) {
  DVLOG(6) << "Receiving " << c;
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
      c->reference_count = 1;
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
}

void Communicator::process_collectives() {
  if( collective_context ) {
    auto c = collective_context;
    int flag;
    MPI_Status status;
    MPI_CHECK( MPI_Test( &c->request, &flag, &status ) );
    if( flag ) {
      c->reference_count = 0;
      if( c->callback ) {
        (c->callback)( c, status.MPI_SOURCE, status.MPI_TAG, c->size );
      }
      collective_context = NULL;
    }
  }
}

void Communicator::poll( unsigned int max_receives ) {
  process_received_buffers();
  process_collectives();
  garbage_collect();
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
  
  MPI_CHECK( MPI_Barrier( grappa_comm ) );
  
  // get rid of any outstanding sends
  for( int i = 0; i < (1 << FLAGS_log2_concurrent_sends); ++i ) {
    if( NULL != sends[i].callback ) {
      sends[i].callback = NULL;
      MPI_CHECK( MPI_Cancel( &sends[i].request ) );
    }
  }
  
  MPI_CHECK( MPI_Barrier( grappa_comm ) );
  
  // get rid of any outstanding receives
  for( int i = 0; i < (1 << FLAGS_log2_concurrent_receives); ++i ) {
    if( NULL != receives[i].callback ) {
      receives[i].callback = NULL;
      MPI_CHECK( MPI_Cancel( &receives[i].request ) );
    }
  }
  
  MPI_CHECK( MPI_Barrier( grappa_comm ) );

  MPI_Comm_free( &locale_comm );
  MPI_Comm_free( &grappa_comm );

  MPI_CHECK( MPI_Barrier( MPI_COMM_WORLD ) );
  
  // For VampirTrace's sake, clean up MPI only when process exits.
  // atexit(finalize_mpi);
  MPI_CHECK( MPI_Finalize() );
}
