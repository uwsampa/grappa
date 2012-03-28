
#include "Cache.hpp"

#include "GlobalAllocator.hpp"
#include "Delegate.hpp"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE( Cache_tests );

int64_t foo = 1234;
int64_t bar[4] = { 2345, 3456, 4567, 6789 };

void user_main( thread * me, void * args ) 
{
  {
    Incoherent< int64_t >::RO buf( GlobalAddress< int64_t >::TwoDimensional( &foo ), 1 );
    BOOST_CHECK_EQUAL( *buf, foo );
  }

  {
    {
      Incoherent<int64_t>::RW buf( GlobalAddress< int64_t >::TwoDimensional( &foo ), 1 );
      BOOST_CHECK_EQUAL( *buf, foo );
      foo = 1235;
      BOOST_CHECK( *buf != foo );
      BOOST_CHECK_EQUAL( foo, 1235 );
      *buf = 1235;
      BOOST_CHECK_EQUAL( *buf, foo );
      *buf = 1236;
    }
    BOOST_CHECK_EQUAL( foo, 1236 );
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


    SoftXMT_free( array );
  }

  SoftXMT_signal_done();
}

BOOST_AUTO_TEST_CASE( test1 ) {

  SoftXMT_init( &(boost::unit_test::framework::master_test_suite().argc),
                &(boost::unit_test::framework::master_test_suite().argv),
                1 << 23);

  SoftXMT_activate();

  BOOST_CHECK_EQUAL( SoftXMT_nodes(), 2 );
  SoftXMT_run_user_main( &user_main, NULL );
  BOOST_CHECK( SoftXMT_done() == true );

  SoftXMT_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();

