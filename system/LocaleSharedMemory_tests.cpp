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

#include <gflags/gflags.h>
#include <glog/logging.h>

#include <boost/test/unit_test.hpp>

#include "Grappa.hpp"
#include "LocaleSharedMemory.hpp"
#include "ParallelLoop.hpp"

BOOST_AUTO_TEST_SUITE( LocaleSharedMemory_tests );

BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa::init( GRAPPA_TEST_ARGS, 1<<10 );
  Grappa::run([]{

    CHECK_EQ( Grappa::locales(), 1 );
    CHECK_GE( Grappa::cores(), 2 );
      
    int64_t * arr = NULL;

    LOG(INFO) << "Allocating";
  
    // allocate array
    if( Grappa::locale_mycore() == 0 ) {
      // we need to do this on each node (not core)
      arr = static_cast< int64_t* >( Grappa::impl::locale_shared_memory.allocate_aligned( sizeof(int64_t) * Grappa::locale_cores(), 1<<3 ) );
      for( int i = 0; i < Grappa::locale_cores(); ++i ) {
        arr[i] = 0;
      }
    }

    LOG(INFO) << "Setting address";
    Grappa::on_all_cores( [arr] {
        int other_index = Grappa::locale_cores() - Grappa::locale_mycore() - 1;
        arr[ other_index ] = Grappa::locale_mycore();
      });

    LOG(INFO) << "Checking";
    Grappa::on_all_cores( [arr] {
        int other_index = Grappa::locale_cores() - Grappa::locale_mycore() - 1;
        BOOST_CHECK_EQUAL( arr[ Grappa::locale_mycore() ], other_index );
      });

    LOG(INFO) << "Done";
  });
  Grappa::finalize();
}

BOOST_AUTO_TEST_SUITE_END();

