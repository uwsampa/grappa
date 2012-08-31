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

#define N 32

LOOP_FUNCTOR(find_local_frontier, nid, ((GlobalAddress<int64_t>,array_))) {
  array = array_;
  int64_t block_elems = block_size / sizeof(int64_t);
  VLOG(1) << "block_elems = " << block_elems; // 8
  
  GlobalAddress<int64_t> base = array;
  int64_t * block_base = base.block_min().pointer();
  VLOG(1) << "base.node() == " << base.node() << ", block_base = " << block_base;
  if (nid < base.node()) {
    local_base = block_base+block_elems;
  } else if (nid > base.node()) {
    local_base = block_base;
  } else {
    local_base = base.pointer();
  }
  GlobalAddress<int64_t> max = array+N;
  block_base = max.block_min().pointer();
  VLOG(1) << "max.node() == " << max.node() << ", block: " << block_base << " -> " << block_base+block_elems;
  int64_t * local_max;
  if (nid < max.node()) {
    local_max = block_base+block_elems;
  } else if (nid > max.node()) {
    local_max = block_base;
  } else {
    local_max = max.pointer();
  }

  int64_t nlocal = local_max - local_base;
  if (local_max < local_base) {
    nlocal = 0;
  }

  VLOG(1) << "local_base = " << local_base << ", local_max = " << local_max << ", nlocal = " << nlocal;

  for (size_t i=0; i<nlocal; i++) {
    local_base[i] = 1;
  }

}

inline void log_array(const char * name, GlobalAddress<int64_t> array) {
  std::stringstream ss; ss << name << ": [";
  for (size_t i=0; i<N; i++) {
    ss << " " << read(array+i);
  }
  ss << " ]";
  VLOG(1) << ss.str();
}

void user_main( void * ignore ) {
  array = SoftXMT_typed_malloc<int64_t>(N);

  SoftXMT_memset(array, 0L, N);
  log_array("array", array);

  { find_local_frontier f(array); fork_join_custom(&f); }
  log_array("array", array);
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

