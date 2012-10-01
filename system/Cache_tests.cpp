
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

/// Tests for Explicit Caches

#include "Cache.hpp"

#include "GlobalAllocator.hpp"
#include "Delegate.hpp"
#include "Tasking.hpp"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE( Cache_tests );

int64_t foo = 1234;
int64_t bar[4] = { 2345, 3456, 4567, 6789 };

struct BrandonM {
  int8_t foo[36];
};

BrandonM brandonm_arr[ 8 ] __attribute__ ((aligned (1 << 12)));

void user_main( int * args ) 
{
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
      int64_t foo1 = SoftXMT_delegate_read_word( GlobalAddress< int64_t >::TwoDimensional( &foo, 1 ) );
      BOOST_CHECK_EQUAL( *buf, foo1 );
      SoftXMT_delegate_write_word( GlobalAddress< int64_t >::TwoDimensional( &foo, 1 ), 1235 );
      foo1 = SoftXMT_delegate_read_word( GlobalAddress< int64_t >::TwoDimensional( &foo, 1 ) );
      BOOST_CHECK( *buf != foo1 );
      BOOST_CHECK_EQUAL( foo1, 1235 );
      *buf = 1235;
      BOOST_CHECK_EQUAL( *buf, foo1 );
      *buf = 1236;
    }
    int64_t foo1 = SoftXMT_delegate_read_word( GlobalAddress< int64_t >::TwoDimensional( &foo, 1 ) );
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
        SoftXMT_yield(); // spin to let reply come back
    }
    buf4.block_until_acquired( );

    buf4.start_release( );
    for (int i=0; i<2000; i++) {
        SoftXMT_yield(); // spin to let reply come back
    }
    buf4.block_until_released( );
  }


  {
    const size_t array_size = 128;
    LOG(INFO) << "Mallocing " << array_size << " words";
    GlobalAddress< int64_t > array = SoftXMT_typed_malloc< int64_t >( array_size );
    
    // write indices properly
    LOG(INFO) << "writing indices";
    for( int i = 0; i < array_size; ++i ) {
      SoftXMT_delegate_write_word( array + i, i );
    }

    // check that we can read a word at a time
    LOG(INFO) << "verifying word reads";
    for( int i = 0; i < array_size; ++i ) {
      LOG(INFO) << "checking element " << i;
      Incoherent<int64_t>::RO d( array + i, 1 );
      BOOST_CHECK_EQUAL( i, *d );
      int64_t dd = SoftXMT_delegate_read_word( array + i );
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
	  BOOST_CHECK_EQUAL( 2, SoftXMT_delegate_read_word( array + start + 0 ) );
	  BOOST_CHECK_EQUAL( 3, SoftXMT_delegate_read_word( array + start + 1 ) );
	  BOOST_CHECK_EQUAL( 4, SoftXMT_delegate_read_word( array + start + 2 ) );
	  BOOST_CHECK_EQUAL( 5, SoftXMT_delegate_read_word( array + start + 3 ) );
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
      int64_t dd = SoftXMT_delegate_read_word( array + i );
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
        int64_t v = SoftXMT_delegate_read_word(array+i);
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
      	  wo.reset( array+i+1, 1 );
      	}
      }

      LOG(INFO) << "read-write reset tests";
      { 
      	Incoherent< int64_t >::RW rw( array, 1 );
      	for (int64_t i=0; i<my_array_size; i++) {
      	  BOOST_CHECK_EQUAL( 2345+i, *rw );
      	  *rw = 3456+i;
      	  rw.reset( array+i+1, 1 );
      	}
      }

      LOG(INFO) << "read-only reset tests";
      {
      	Incoherent< int64_t >::RO ro( array, 1 );
      	for (int64_t i=0; i<my_array_size; i++) {
      	  BOOST_CHECK_EQUAL( 3456+i, *ro );
      	  ro.reset( array+i+1, 1 );
      	}

  	// check address()
  	BOOST_CHECK_EQUAL( ro.address(), array + my_array_size );
      }

    }

    // make sure zero-length cache objects don't fail
    Incoherent< int64_t >::RW zero_length( make_global( &foo ), 0 );
    zero_length.block_until_acquired();
    BOOST_CHECK_EQUAL( zero_length.address(), make_global( &foo ) );
    zero_length.block_until_released();

    // make sure non-word-multiple cache objects don't fail
    BOOST_MESSAGE( "brandonm 36-byte request bug tests" );
    Incoherent< BrandonM >::RW brandonm( make_global( &brandonm_arr, 1 ), 1 );
    brandonm.block_until_acquired();
    BOOST_CHECK_EQUAL( brandonm.address(), make_global( &brandonm_arr, 1 ) );
    brandonm.block_until_released();

    // make sure non-word-multiple cache objects don't fail
    Incoherent< BrandonM >::WO brandonmwo( make_global( &brandonm_arr, 1 ), 1 );
    brandonmwo.block_until_acquired();
    BrandonM brandonm_val = { 123 };
    *brandonmwo = brandonm_val;
    BOOST_CHECK_EQUAL( brandonmwo.address(), make_global( &brandonm_arr, 1 ) );
    brandonmwo.block_until_released();

    // make sure non-word-multiple cache objects don't fail
    BOOST_MESSAGE( "brandonm bug direct tests" );
    LOG(INFO) << "brandonm bug direct tests";
    GlobalAddress< BrandonM > brandonmx_addr = SoftXMT_typed_malloc< BrandonM >( 5 );
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

    SoftXMT_free( brandonmx_addr );

    SoftXMT_free( array );
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
  //SoftXMT_waitForTasks();
  //SoftXMT_signal_done();

  SoftXMT_dump_stats_all_nodes();
}

BOOST_AUTO_TEST_CASE( test1 ) {

  SoftXMT_init( &(boost::unit_test::framework::master_test_suite().argc),
                &(boost::unit_test::framework::master_test_suite().argv),
                1 << 23);

  SoftXMT_activate();

  BOOST_CHECK_EQUAL( SoftXMT_nodes(), 2 );
  SoftXMT_run_user_main( &user_main, (int*)NULL );
  BOOST_CHECK( SoftXMT_done() == true );

  SoftXMT_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();

