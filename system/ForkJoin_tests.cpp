#include <boost/test/unit_test.hpp>
#include "SoftXMT.hpp"
#include "Addressing.hpp"
#include "Cache.hpp"
#include "GlobalAllocator.hpp"
#include "ForkJoin.hpp"

BOOST_AUTO_TEST_SUITE( ForkJoin_tests );

#define MEM_SIZE 1<<24;

struct func_initialize : public ForkJoinIteration {
  GlobalAddress<int64_t> base_addr;
  int64_t value;
  void operator()(Thread * me, int64_t index) {
    VLOG(2) << "called func_initialize with index = " << index;
    Incoherent<int64_t>::RW c(base_addr+index, 1);
    c[0] = value+index;
  }
};

struct func_hello : public ForkJoinIteration {
  void operator()(Thread * me, int64_t index) {
    LOG(INFO) << "Hello from " << index << "!";
  }
};

static void user_main(Thread * me, void * args) {
  
  LOG(INFO) << "beginning user main... (" << SoftXMT_mynode() << ")";
  
  {
    size_t N = 128;
    GlobalAddress<int64_t> data = SoftXMT_typed_malloc<int64_t>(N);
    
    func_initialize a; a.base_addr = data; a.value = 0;
    fork_join(me, &a, 0, N);
    
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
    
    func_initialize a; a.base_addr = data; a.value = 0;
    fork_join(me, &a, 0, N);
    
    for (size_t i=0; i<N; i++) {
      Incoherent<int64_t>::RO c(data+i, 1);
      VLOG(2) << i << " == " << *c;
      BOOST_CHECK_EQUAL(i, *c);
    }
    SoftXMT_free(data);
  }
  {
    func_hello f;
    fork_join_custom(me, &f);
  }
  
  
  SoftXMT_signal_done();
}

BOOST_AUTO_TEST_CASE( test1 ) {
  SoftXMT_init( &(boost::unit_test::framework::master_test_suite().argc),
               &(boost::unit_test::framework::master_test_suite().argv), 1<<14);
  SoftXMT_activate();
  
  SoftXMT_run_user_main(&user_main, NULL);
  
  LOG(INFO) << "finishing...";
	SoftXMT_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();
