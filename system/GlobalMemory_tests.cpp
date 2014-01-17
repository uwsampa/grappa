
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <sys/mman.h>

#include <boost/test/unit_test.hpp>

#include "Grappa.hpp"
#include "Delegate.hpp"
#include "Cache.hpp"
#include "GlobalMemory.hpp"

BOOST_AUTO_TEST_SUITE( GlobalMemory_tests );

BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa::init( GRAPPA_TEST_ARGS, (1L << 20) );
  Grappa::run([]{
    size_t N = 128;
    GlobalAddress<int64_t> data = Grappa::global_alloc<int64_t>(N);
  
    for (size_t i=0; i<N; i++) {
      BOOST_MESSAGE( "Writing " << i << " to " << data+i );
      Grappa::delegate::write(data+i, i);
    }
  
    for (size_t i=0; i<N; i++) {
      Incoherent<int64_t>::RO c(data+i, 1);
      VLOG(2) << i << " == " << *c;
      BOOST_CHECK_EQUAL(i, *c);
    }

    Grappa::global_free(data);
  });
  Grappa::finalize();
}

BOOST_AUTO_TEST_SUITE_END();
