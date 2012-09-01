#include <boost/test/unit_test.hpp>

#include <SoftXMT.hpp>
#include <GlobalTaskJoiner.hpp>
#include <ForkJoin.hpp>
#include <AsyncParallelFor.hpp>

#include <Delegate.hpp>
#define read SoftXMT_delegate_read_word
#define write SoftXMT_delegate_write_word

#include <GlobalAllocator.hpp>
#include <PerformanceTools.hpp>


BOOST_AUTO_TEST_SUITE( LocalIterator_tests );

GlobalAddress<int64_t> array;
int64_t * local_base;

const size_t N = (1L<<24);
const size_t nbuf = 1L<<22;

inline void set_0(uint64_t * a) { *a = 0; }

void user_main( void * ignore ) {
  array = SoftXMT_typed_malloc<int64_t>(N);
  int64_t * buf = new int64_t[nbuf];
  
  double t, comm_time, local_time;

  SOFTXMT_TIME(comm_time, {
    SoftXMT_memset(array, 0L, N);
  });

  BOOST_MESSAGE("checking local memset works");
  SOFTXMT_TIME(local_time, {
    SoftXMT_memset_local(array, 1L, N);
  });

  for (size_t i=0; i<N; i+=nbuf) {
    size_t n = MIN(N-i, nbuf);
    Incoherent<int64_t>::RO c(array+i, n, buf);
    for (size_t j=0; j<n; j++) {
      //VLOG(1) << c[j];
      //BOOST_CHECK_EQUAL(c[j], 1);
      CHECK(c[j] == 1) << ">>> j = " << j;
    }
  }

  BOOST_MESSAGE("checking forall_local");
  t = SoftXMT_walltime();
  forall_local<uint64_t,set_0>(array, N);
  double local_for_time = SoftXMT_walltime() - t;
  
  for (size_t i=0; i<N; i+=nbuf) {
    size_t n = MIN(N-i, nbuf);
    Incoherent<int64_t>::RO c(array+i, n, buf);
    for (size_t j=0; j<n; j++) {
      //VLOG(1) << c[j];
      BOOST_CHECK_EQUAL(c[j], 0);
      CHECK(c[j] == 0) << ">>> j = " << j;
    }
  }

  VLOG(1) << "local_memset_time: " << local_time << ", local_for_time: " << local_for_time;
}

BOOST_AUTO_TEST_CASE( test1 ) {

  SoftXMT_init( &(boost::unit_test::framework::master_test_suite().argc),
                &(boost::unit_test::framework::master_test_suite().argv) );

  SoftXMT_activate();

  SoftXMT_run_user_main( &user_main, (void*)NULL );
  CHECK( SoftXMT_done() );

  SoftXMT_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();

