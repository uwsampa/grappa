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

/// Tests for Explicit Caches

#include <boost/test/unit_test.hpp>
#include <Grappa.hpp>
#include <Cache.hpp>

namespace Grappa {
namespace impl {
extern void * global_memory_chunk_base;
}
}


BOOST_AUTO_TEST_SUITE( Cache_tests );

struct BrandonM {
  int8_t foo[36];
};

BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa::init( GRAPPA_TEST_ARGS );

  BOOST_CHECK_EQUAL( Grappa::cores(), 2 );

  Grappa::run([]{
    // totally cheat; use the local 
    char * local = reinterpret_cast< char* >( ::Grappa::impl::global_memory_chunk_base ) + 4096;

    // make sure this is aligned
    BrandonM * brandonm_arr = reinterpret_cast< BrandonM* >( local );
    local += 8 * sizeof( BrandonM );

    int64_t & foo = *(reinterpret_cast< int64_t* >( local ));
    foo = 1234;
    local += sizeof(int64_t);
  
    int64_t * bar = reinterpret_cast< int64_t* >( local );
    bar[0] = 2345;
    bar[1] = 3456;
    bar[2] = 4567;
    bar[3] = 6789;
    local += 4 * sizeof(int64_t);

    {
      Incoherent< int64_t >::RO buf( GlobalAddress< int64_t >::TwoDimensional( &foo ), 1 );
      BOOST_CHECK_EQUAL( *buf, foo );
    }

    { // local. watch out for short-circuit
      {
        Incoherent<int64_t>::RW buf( GlobalAddress< int64_t >::TwoDimensional( &foo ), 1 );
        BOOST_CHECK_EQUAL( *buf, foo );
        foo = 1235;
        BOOST_CHECK_EQUAL( *buf, foo );
        *buf = 1236;
      }
      BOOST_CHECK_EQUAL( foo, 1236 );
    }

    { // remote. no short-circuit.
      {
        Incoherent<int64_t>::RW buf( GlobalAddress< int64_t >::TwoDimensional( &foo, 1 ), 1 );
        int64_t foo1 = Grappa::delegate::read( GlobalAddress< int64_t >::TwoDimensional( &foo, 1 ) );
        BOOST_CHECK_EQUAL( *buf, foo1 );
        Grappa::delegate::write( GlobalAddress< int64_t >::TwoDimensional( &foo, 1 ), 1235 );
        foo1 = Grappa::delegate::read( GlobalAddress< int64_t >::TwoDimensional( &foo, 1 ) );
        BOOST_CHECK( *buf != foo1 );
        BOOST_CHECK_EQUAL( foo1, 1235 );
        *buf = 1235;
        BOOST_CHECK_EQUAL( *buf, foo1 );
        *buf = 1236;
      }
      int64_t foo1 = Grappa::delegate::read( GlobalAddress< int64_t >::TwoDimensional( &foo, 1 ) );
      BOOST_CHECK_EQUAL( foo1, 1236 );
    }

    {
      int64_t x;
      Incoherent<int64_t>::RO buf( GlobalAddress< int64_t >::TwoDimensional( &foo ), 1, &x );
      BOOST_CHECK_EQUAL( *buf, foo );
    }

    {
      int64_t y[4];
      Incoherent<int64_t>::RO buf2( GlobalAddress< int64_t >::TwoDimensional( bar ), 4, &y[0] );
      BOOST_CHECK_EQUAL( buf2[2], bar[2] );
    }

    {
      Incoherent<int64_t>::RW buf3( GlobalAddress< int64_t >::TwoDimensional( bar ), 4);
      BOOST_CHECK_EQUAL( buf3[2], bar[2] );
    }

    {
      // test for early wakeup handled properly
      BOOST_MESSAGE( "Wakeup test" );
      Incoherent<int64_t>::RW buf4( GlobalAddress< int64_t >::TwoDimensional( bar, 1 ), 1);
      buf4.start_acquire( );
      for (int i=0; i<2000; i++) {
          Grappa::yield(); // spin to let reply come back
      }
      buf4.block_until_acquired( );

      buf4.start_release( );
      for (int i=0; i<2000; i++) {
          Grappa::yield(); // spin to let reply come back
      }
      buf4.block_until_released( );
    }


    {
      const size_t array_size = 128;
      LOG(INFO) << "Mallocing " << array_size << " words";
      GlobalAddress< int64_t > array = Grappa::global_alloc< int64_t >( array_size );
    
      // write indices properly
      LOG(INFO) << "writing indices";
      for( int i = 0; i < array_size; ++i ) {
        Grappa::delegate::write( array + i, i );
      }

      // check that we can read a word at a time
      LOG(INFO) << "verifying word reads";
      for( int i = 0; i < array_size; ++i ) {
        LOG(INFO) << "checking element " << i;
        Incoherent<int64_t>::RO d( array + i, 1 );
        BOOST_CHECK_EQUAL( i, *d );
        int64_t dd = Grappa::delegate::read( array + i );
        BOOST_CHECK_EQUAL( i, dd );
        BOOST_CHECK_EQUAL( dd, *d );
      }

      // check that we can read two words at a time
      // LOG(INFO) << "verifying block reads";
      // for( int i = 0; i < array_size; ++i ) {
      //   Incoherent<int64_t>::RO d( array + i, 2 );
      //   BOOST_CHECK_EQUAL( i, d[0] );
      //   if( i+1 < array_size ) BOOST_CHECK_EQUAL( i+1, d[1] );
      // }

    
      LOG(INFO) << "verifying block reads";
      for( int size = 1; size < array_size; ++size ) {
        LOG(INFO) << "checking blocks of size " << size; 
        for( int start = 0; start < array_size; ++start ) {
  	//LOG(INFO) << "checking from start " << start;
  	int actual_size = ( start + size > array_size ? array_size : start + size ) - start;
  	DVLOG(5) << "starting iteration with start:" << start 
  		 << " size:" << size 
  		 << " actual_size:" << actual_size;
	
  	DVLOG(5) << "*****************************************************************************";
  	//start:2 size:4 actual_size:4 i:2
  	if( start == 2 && size == 4 ) {
  	  BOOST_CHECK_EQUAL( 2, Grappa::delegate::read( array + start + 0 ) );
  	  BOOST_CHECK_EQUAL( 3, Grappa::delegate::read( array + start + 1 ) );
  	  BOOST_CHECK_EQUAL( 4, Grappa::delegate::read( array + start + 2 ) );
  	  BOOST_CHECK_EQUAL( 5, Grappa::delegate::read( array + start + 3 ) );
  	}
  	DVLOG(5) << "*****************************************************************************";
	
  	{
  	  Incoherent<int64_t>::RW d( array + start, actual_size );
  	  d.block_until_acquired();
  	  DVLOG(5) << "*****************************************************************************";
  	  for( int i = start; i < actual_size; ++i ) {
  	    DVLOG(5) << "checking word with start:" << start 
  		     << " size:" << size 
  		     << " actual_size:" << actual_size 
  		     << " i:" << i;
  	    BOOST_CHECK_EQUAL( i, d[i - start] );
  	    CHECK_EQ( i, d[i - start] ) << "start:" << start 
  					<< " size:" << size 
  					<< " actual_size:" << actual_size 
  					<< " i:" << i;
  	    d[i - start] = i;
  	  }
  	}
        }
      }

      // check that we can read the words that were written
      LOG(INFO) << "verifying word writes";
      for( int i = 0; i < array_size; ++i ) {
        LOG(INFO) << "checking element " << i;
        //Incoherent<int64_t>::RO d( array + i, 1 );
        //BOOST_CHECK_EQUAL( i, *d );
        int64_t dd = Grappa::delegate::read( array + i );
        BOOST_CHECK_EQUAL( i, dd );
        //BOOST_CHECK_EQUAL( dd, *d );
      }

      {
        BOOST_MESSAGE("Write-only tests");
        {
          BOOST_MESSAGE("WO block test");
          Incoherent<int64_t>::WO c(array, array_size);
          for (int64_t i=0; i<array_size; i++) {
            c[i] = 1234+i;
          }
        }
        for (int64_t i=0; i<array_size; i++) {
          int64_t v = Grappa::delegate::read(array+i);
          BOOST_CHECK_EQUAL(v, 1234+i);
        }
      
        {
          LOG(INFO) << "WO short-circuit test";
          LOG(INFO) << "&foo = " << &foo;
          Incoherent<int64_t>::WO cfoo(make_global(&foo), 1);
          *cfoo = 321;
        }
        BOOST_CHECK_EQUAL(321, foo);
      }

      {
        LOG(INFO) << "reset tests";

        int64_t my_array_size = array_size;

        // make sure array is initialized the way we expect
        {
        	Incoherent<int64_t>::WO c(array, array_size);
        	for (int64_t i=0; i<array_size; i++) {
        	  c[i] = 1234+i;
        	}
        }

        LOG(INFO) << "write-only reset tests";
        {
        	Incoherent< int64_t >::WO wo( array, 1 );
        	for (int64_t i=0; i<my_array_size; i++) {
        	  *wo = 2345+i;
        
            if ( i<my_array_size-1 ) { 
              wo.reset( array+i+1, 1 );
            }
        	}
        }

        LOG(INFO) << "read-write reset tests";
        { 
        	Incoherent< int64_t >::RW rw( array, 1 );
        	for (int64_t i=0; i<my_array_size; i++) {
        	  BOOST_CHECK_EQUAL( 2345+i, *rw );
        	  *rw = 3456+i;
      	  
            if ( i<my_array_size-1 ) { 
              rw.reset( array+i+1, 1 );
            }
        	}
        }

        LOG(INFO) << "read-only reset tests";
        {
        	Incoherent< int64_t >::RO ro( array, 1 );
        	for (int64_t i=0; i<my_array_size; i++) {
        	  BOOST_CHECK_EQUAL( 3456+i, *ro );

            if ( i<my_array_size-1 ) {
              ro.reset( array+i+1, 1 );
            }
        	}

    	// check address()
    	BOOST_CHECK_EQUAL( ro.address(), array + my_array_size-1 );
        }

      }

      // make sure zero-length cache objects don't fail
      Incoherent< int64_t >::RW zero_length( make_global( &foo ), 0 );
      zero_length.block_until_acquired();
      BOOST_CHECK_EQUAL( zero_length.address(), make_global( &foo ) );
      zero_length.block_until_released();

      // make sure non-word-multiple cache objects don't fail
      BOOST_MESSAGE( "brandonm 36-byte request bug tests" );
      Incoherent< BrandonM >::RW brandonm( make_global( brandonm_arr, 1 ), 1 );
      brandonm.block_until_acquired();
      BOOST_CHECK_EQUAL( brandonm.address(), make_global( brandonm_arr, 1 ) );
      brandonm.block_until_released();

      // make sure non-word-multiple cache objects don't fail
      Incoherent< BrandonM >::WO brandonmwo( make_global( brandonm_arr, 1 ), 1 );
      brandonmwo.block_until_acquired();
      BrandonM brandonm_val = { 123 };
      *brandonmwo = brandonm_val;
      BOOST_CHECK_EQUAL( brandonmwo.address(), make_global( brandonm_arr, 1 ) );
      brandonmwo.block_until_released();

      // make sure non-word-multiple cache objects don't fail
      BOOST_MESSAGE( "brandonm bug direct tests" );
      LOG(INFO) << "brandonm bug direct tests";
      GlobalAddress< BrandonM > brandonmx_addr = Grappa::global_alloc< BrandonM >( 5 );
      // BOOST_MESSAGE( "brandonmx address is " << brandonmx_addr );
      // LOG(INFO) << "brandonmx address is " << brandonmx_addr;

      // should be 1 block
      LOG(INFO) << "should be 1 block";
      Incoherent< BrandonM >::WO brandonmx( brandonmx_addr, 1 );
      brandonmx.block_until_acquired();
      brandonmx.block_until_released();

      // should be 2 blocks
      LOG(INFO) << "should be 2 blocks";
      Incoherent< BrandonM >::WO brandonmx1( brandonmx_addr + 1, 1 );
      brandonmx1.block_until_acquired();
      brandonmx1.block_until_released();

      // should be 2 blocks
      LOG(INFO) << "should be 2 blocks";
      Incoherent< BrandonM >::WO brandonmx2( brandonmx_addr, 3 );
      brandonmx2.block_until_acquired();
      brandonmx2.block_until_released();

      // should be 3 blocks
      LOG(INFO) << "should be 3 blocks";
      Incoherent< BrandonM >::WO brandonmx3( brandonmx_addr + 1, 3 );
      brandonmx3.block_until_acquired();
      brandonmx3.block_until_released();

      Grappa::global_free( brandonmx_addr );

      Grappa::global_free( array );
    }

    {
      BOOST_MESSAGE("Empty cache test");

      int64_t x = 0, y;
      GlobalAddress<int64_t> xa = make_global(&x);
      int64_t buf;
      LOG(INFO) << "empty RO...";
      { Incoherent<int64_t>::RO c(xa, 1, &buf); y = c[0]; }
    
      LOG(INFO) << "empty WO...";
      { Incoherent<int64_t>::WO c(xa, 1, &buf); c[0] = 1; }
    
      LOG(INFO) << "empty RW...";
      { Incoherent<int64_t>::RW c(xa, 1, &buf); c[0] = c[0]+1; }
    }
    
    Grappa::Metrics::merge_and_print();
  });
  Grappa::finalize();
}

BOOST_AUTO_TEST_SUITE_END();
