#include <boost/test/unit_test.hpp>
#include "SoftXMT.hpp"
#include "Addressing.hpp"
#include "Cache.hpp"
#include "ForkJoin.hpp"
#include "GlobalAllocator.hpp"

BOOST_AUTO_TEST_SUITE( ForkJoin_tests );

#define MEM_SIZE 1L<<24;

LOOP_FUNCTOR( func_initialize, index, ((GlobalAddress<int64_t>, base_addr)) ((int64_t, value)) ) {
  VLOG(2) << "called func_initialize with index = " << index;
  Incoherent<int64_t>::RW c(base_addr+index, 1);
  c[0] = value+index;
}

LOOP_FUNCTION( func_hello, index ) {
  LOG(INFO) << "Hello from " << index << "!";
}

static void user_main(int * args) {
  
  LOG(INFO) << "beginning user main.... (" << SoftXMT_mynode() << ")";
  
  {
    size_t N = 128;
    GlobalAddress<int64_t> data = SoftXMT_typed_malloc<int64_t>(N);
    
    func_initialize a(data, 0);
    fork_join(&a, 0, N);
    
    for (size_t i=0; i<N; i++) {
      Incoherent<int64_t>::RO c(data+i, 1);
      VLOG(2) << i << " == " << *c;
      BOOST_CHECK_EQUAL(i, *c);
    }
    SoftXMT_free(data);
  }
  {
    size_t N = 101;
    GlobalAddress<int64_t> data = SoftXMT_typed_malloc<int64_t>(N);
    
    func_initialize a(data, 0);
    fork_join(&a, 0, N);
    
    for (size_t i=0; i<N; i++) {
      Incoherent<int64_t>::RO c(data+i, 1);
      VLOG(2) << i << " == " << *c;
      BOOST_CHECK_EQUAL(i, *c);
    }
    SoftXMT_free(data);
  }
  {
    func_hello f;
    fork_join_custom(&f);
  }
  {
    size_t N = 128 + 13;
    GlobalAddress<int64_t> data = SoftXMT_typed_malloc<int64_t>(N);
    
    func_initialize a(data, 0);
    fork_join(&a, 0, N);
   
	VLOG(2) << "done with init";

    for (size_t i=0; i<N; i++) {
      Incoherent<int64_t>::RO c(data+i, 1);
      VLOG(2) << i << " == " << *c;
      BOOST_CHECK_EQUAL(i, *c);
    }
    SoftXMT_free(data);
  }
  
}

BOOST_AUTO_TEST_CASE( test1 ) {
  SoftXMT_init( &(boost::unit_test::framework::master_test_suite().argc),
               &(boost::unit_test::framework::master_test_suite().argv), 1<<20);
  SoftXMT_activate();
  
  SoftXMT_run_user_main(&user_main, (int*)NULL);
  
  LOG(INFO) << "finishing...";
	SoftXMT_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();
