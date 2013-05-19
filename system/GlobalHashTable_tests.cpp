
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <boost/test/unit_test.hpp>
#include "Grappa.hpp"
#include "ParallelLoop.hpp"
#include "GlobalAllocator.hpp"
#include "Delegate.hpp"
#include "GlobalHashTable.hpp"
#include "Statistics.hpp"

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/variate_generator.hpp>

using namespace Grappa;

BOOST_AUTO_TEST_SUITE( GlobalHashTable_tests );

DEFINE_int64(nelems, 100, "number of elements in (large) test arrays");
DEFINE_int64(ght_size, 1024, "number of elements in (large) test arrays");

DEFINE_int64(ntrials, 1, "number of independent trials to average over");

DEFINE_bool(perf, false, "do performance test");

GRAPPA_DEFINE_STAT(SummarizingStatistic<double>, ght_insert_time, 0);

template< typename T >
inline T next_random() {
  using engine_t = boost::mt19937_64;
  using dist_t = boost::uniform_int<T>;
  using gen_t = boost::variate_generator<engine_t&,dist_t>;
  
  // continue using same generator with multiple calls (to not repeat numbers)
  // but start at different seed on each node so we don't get overlap
  static engine_t engine(12345L*mycore());
  static gen_t gen(engine, dist_t(0, std::numeric_limits<T>::max()));
  return gen();
}

uint64_t kahan_hash(long k) { return k * 1299227; } // "Kahan's Hash"
uint64_t identity_hash(long k) { return k; }

enum class Exp { INSERT_UNIQUE };

template< Exp EXP,
          typename K, typename V, uint64_t (*H)(K),
          CompletionEvent* CE = &impl::local_ce,
          int64_t          TH = impl::USE_LOOP_THRESHOLD_FLAG >
double test_insert_throughput(GlobalAddress<GlobalHashTable<K,V,H>> ha) {
  double t = Grappa_walltime();
  
  forall_global_private<CE,TH>(0, FLAGS_nelems, [ha](int64_t i){
    if (EXP == Exp::INSERT_UNIQUE) {
      ha->insert_unique(next_random<long>());
    }
  });
  
  t = Grappa_walltime() - t;
  return t;
}

void test_correctness() {
  LOG(INFO) << "Testing correctness...";
  auto ha = GlobalHashTable<long,long,&identity_hash>::create(FLAGS_ght_size);
  for (int i=0; i<10; i++) {
    ha->insert(i, 42);
  }
  ha->global_set_RO();
  for (int i=0; i<10; i++) {
    GlobalAddress<long> res;
    BOOST_CHECK_EQUAL(ha->lookup(i, &res), 1);
    BOOST_CHECK_EQUAL(delegate::read(res), 42);
  }
  
  ha->forall_entries([](long key, BufferVector<long>& vs){ vs.setReadWriteMode(); });
  
  forall_global_private(10, FLAGS_nelems-10, [ha](int64_t i){
    ha->insert(i, 42);
  });
  ha->forall_entries([](long key, BufferVector<long>& vs){ vs.setReadWriteMode(); });

  ha->forall_entries([](long key, BufferVector<long>& vals){
    BOOST_CHECK_EQUAL(vals.size(), 1);
    auto a = vals.getReadBuffer();
    BOOST_CHECK_EQUAL(a.core(), mycore());
    BOOST_CHECK_EQUAL(delegate::read(a), 42);
  });
  
  ha->destroy();
}

void user_main( void * ignore ) {
  if (FLAGS_perf) {
    auto ha = GlobalHashTable<long,long,&kahan_hash>::create(FLAGS_ght_size);
    
    for (int i=0; i<FLAGS_ntrials; i++) {
      ght_insert_time += test_insert_throughput<Exp::INSERT_UNIQUE>(ha);
    }
    
    ha->destroy();
    
  } else {
    test_correctness();
  }
  
  Statistics::merge_and_print();
}

BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa_init( &(boost::unit_test::framework::master_test_suite().argc),
                &(boost::unit_test::framework::master_test_suite().argv) );
  Grappa_activate();
  Grappa_run_user_main( &user_main, (void*)NULL );
  Grappa_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();

