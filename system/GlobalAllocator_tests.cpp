
#include <sys/mman.h>

#include <boost/test/unit_test.hpp>

#include "SoftXMT.hpp"
#include "Delegate.hpp"
#include "GlobalMemoryChunk.hpp"
#include "GlobalAllocator.hpp"

BOOST_AUTO_TEST_SUITE( GlobalAllocator_tests );


const size_t local_size_bytes = 1 << 10;

void test( thread * me, void* args ) {
  GlobalAddress< int64_t > a = SoftXMT_malloc( 1 );
  LOG(INFO) << "got pointer " << a.pointer();

  GlobalAddress< int64_t > b = SoftXMT_typed_malloc< int64_t >( 1 );
  LOG(INFO) << "got pointer " << b.pointer();

  GlobalAddress< int64_t > c = SoftXMT_malloc( 8 );
  LOG(INFO) << "got pointer " << c.pointer();

  GlobalAddress< int64_t > d = SoftXMT_malloc( 1 );
  LOG(INFO) << "got pointer " << d.pointer();

  LOG(INFO) << *global_allocator;

  if( SoftXMT_mynode() == 0 ) {
    BOOST_CHECK_EQUAL( global_allocator->total_bytes(), local_size_bytes * SoftXMT_nodes() );
    BOOST_CHECK_EQUAL( global_allocator->total_bytes_in_use(), 1 + 8 + 8 + 1 );
  }

  LOG(INFO) << "freeing pointer " << c.pointer();
  SoftXMT_free( c );

  LOG(INFO) << "freeing pointer " << a.pointer();
  SoftXMT_free( a );

  LOG(INFO) << "freeing pointer " << d.pointer();
  SoftXMT_free( d );

  LOG(INFO) << "freeing pointer " << b.pointer();
  SoftXMT_free( b );

  if( SoftXMT_mynode() == 0 ) {
    BOOST_CHECK_EQUAL( global_allocator->total_bytes_in_use(), 0 );
  }
  LOG(INFO) << *global_allocator;
  
  GlobalAddress<int64_t> * vargs = reinterpret_cast< GlobalAddress<int64_t> * >( args );
  //SoftXMT_flush( vargs->node() );
  SoftXMT_delegate_fetch_and_add_word( *vargs, 1 );
  SoftXMT_flush( vargs->node() );
}

void user_main( thread * me, void * args ) 
{
  int count = 0;
  GlobalAddress< int64_t > global_count = make_global( &count );
  //SoftXMT_spawn( &test, &global_count );
  //test( NULL, &global_count );
  //test( NULL, &global_count );
  SoftXMT_spawn( &test, &global_count );
  //SoftXMT_spawn( &test, &global_count );
  //SoftXMT_flush( 0 );

  SoftXMT_remote_spawn( &test, &global_count, 1 );

  while( count < 2 ) {
    //LOG(INFO) << "count is " << 2;
    SoftXMT_yield();
  }

  BOOST_CHECK_EQUAL( global_allocator->total_bytes_in_use(), 0 );
  LOG(INFO) << *global_allocator;

  SoftXMT_signal_done();
}

BOOST_AUTO_TEST_CASE( test1 ) {

  SoftXMT_init( &(boost::unit_test::framework::master_test_suite().argc),
                &(boost::unit_test::framework::master_test_suite().argv),
                local_size_bytes * 2);

  assert( SoftXMT_nodes() == 2 );
  SoftXMT_activate();

  DVLOG(1) << "Spawning user main thread....";
  SoftXMT_run_user_main( &user_main, NULL );
  BOOST_CHECK( SoftXMT_done() == true );

  SoftXMT_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();

