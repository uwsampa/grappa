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

/// Tests for communicator

#include "Communicator.hpp"
#include "LocaleSharedMemory.hpp"

/*
 * tests
 */
#include <boost/test/unit_test.hpp>

DECLARE_int64( log2_concurrent_receives );

/// provide some things we'd normally get from other parts of the sysatem

bool freeze_flag = false;

// from google
namespace google {
extern void DumpStackTrace();
}

namespace Grappa {
namespace impl {

void freeze_for_debugger() {
  LOG(INFO) << global_communicator.hostname() << " freezing for debugger. Set freeze_flag=false to continue.";
  google::FlushLogFiles(google::GLOG_INFO);
  fflush(stdout);
  fflush(stderr);

  while( freeze_flag ) {
    sleep(1);
  }
}

void failure_function() {
  google::FlushLogFiles(google::GLOG_INFO);
  google::DumpStackTrace();
  if( freeze_flag ) {
    freeze_for_debugger();
  }
  exit(1);
}

/// how much memory do we expect to allocate?
int64_t global_memory_size_bytes = 1 << 24;
int64_t global_bytes_per_core = 1 << 23;
int64_t global_bytes_per_locale = 1 << 23;

}
}



BOOST_AUTO_TEST_SUITE( Communicator_tests );

bool success = false;
bool sent = false;

int send_count = 1 << 22;
int receive_count = 0;


void ping_test() {
  int target = (Grappa::mycore() + ( Grappa::cores() / 2 ) ) % Grappa::cores();

  double start = MPI_Wtime();
  MPI_CHECK( MPI_Barrier( global_communicator.grappa_comm ) );

  if( Grappa::mycore() < Grappa::cores() / 2 ) {
    for( int i = 0; i < send_count; ++i ) {
      global_communicator.send_immediate( target, [] {
          receive_count++;
          DVLOG(1) << "Receive count now " << receive_count;
        });
    }
    receive_count = send_count;
  } else {
    while( receive_count != send_count ) {
      global_communicator.poll();
    }
  }

  DVLOG(1) << "Done.";
  
  MPI_CHECK( MPI_Barrier( global_communicator.grappa_comm ) );
  double end = MPI_Wtime();

  BOOST_CHECK_EQUAL( send_count, receive_count );

  if( Grappa::mycore() == 0 ) {
    double time = end - start;
    double rate = send_count * ( Grappa::cores() / 2 ) / time;
    LOG(INFO) << send_count << " messages in "
              << time << " seconds: "
              << rate << " msgs/s";
  }
}

void payload_test() {

  send_count = 12345678;
  receive_count = 0;

  MPI_CHECK( MPI_Barrier( global_communicator.grappa_comm ) );

  BOOST_CHECK_EQUAL( receive_count, 0 );

  MPI_CHECK( MPI_Barrier( global_communicator.grappa_comm ) );

  if( Grappa::mycore() == 0 ) {
    size_t size = sizeof(send_count);
    global_communicator.send_immediate_with_payload( 1, [size] (void * buf, int size) {
        int * ptr = (int*) buf;
        LOG(INFO) << "Got payload message with pointer " << ptr
                  << " size " << size
                  << " value " << *ptr;
        receive_count = *ptr;
        success = true;
      }, &send_count, size );
    receive_count = send_count;
  } else {
    while( !success ) {
      global_communicator.poll();
    }
  }

  MPI_CHECK( MPI_Barrier( global_communicator.grappa_comm ) );

  BOOST_CHECK_EQUAL( send_count, receive_count );
}
  
BOOST_AUTO_TEST_CASE( test1 ) {
  google::ParseCommandLineFlags( &(boost::unit_test::framework::master_test_suite().argc),
                                 &(boost::unit_test::framework::master_test_suite().argv), true );
  google::InitGoogleLogging( boost::unit_test::framework::master_test_suite().argv[0] );

  global_communicator.init( &(boost::unit_test::framework::master_test_suite().argc),
             &(boost::unit_test::framework::master_test_suite().argv) );
  Grappa::impl::locale_shared_memory.init();
  Grappa::impl::locale_shared_memory.activate();

  global_communicator.activate();
  
  MPI_CHECK( MPI_Barrier( global_communicator.grappa_comm ) );

  ping_test();

  MPI_CHECK( MPI_Barrier( global_communicator.grappa_comm ) );

  payload_test();
  
  MPI_CHECK( MPI_Barrier( global_communicator.grappa_comm ) );

  BOOST_CHECK_EQUAL( true, true );

  global_communicator.finish();
}

BOOST_AUTO_TEST_SUITE_END();

