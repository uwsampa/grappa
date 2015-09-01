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

#include <boost/test/unit_test.hpp>
#include "Grappa.hpp"
#include "ParallelLoop.hpp"
#include "GlobalAllocator.hpp"
#include "Delegate.hpp"
#include "GlobalHashMap.hpp"
#include "GlobalHashSet.hpp"
#include "Metrics.hpp"

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/variate_generator.hpp>
#include <random>

using namespace Grappa;

BOOST_AUTO_TEST_SUITE( GlobalHash_tests );

DEFINE_int64(nelems, 100, "number of elements in (large) test arrays");
DEFINE_int64(global_hash_size, 1024, "number of elements in (large) test arrays");
DEFINE_int64(max_key, 1<<10, "maximum random key");

DEFINE_int64(ntrials, 1, "number of independent trials to average over");

DEFINE_bool(map_perf, false, "do performance test of GlobalHashMap");
DEFINE_bool(set_perf, false, "do performance test of GlobalHashSet");

DEFINE_bool(insert_async, false, "do async inserts");

DEFINE_double(fraction_lookups, 0.0, "fraction of accesses that should be lookups");

GRAPPA_DEFINE_METRIC(SummarizingMetric<double>, trial_time, 0);

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

bool choose_random(double probability) {
  static std::default_random_engine e(12345L*mycore());
  static std::uniform_real_distribution<double> dist(0.0, 1.0);
  auto v = dist(e);
  return v < probability;
}

uint64_t kahan_hash(long k) { return k * 1299227; } // "Kahan's Hash"
uint64_t identity_hash(long k) { return k; }

enum class Exp { INSERT };

template< Exp EXP,
          typename K, typename V,
          int64_t          TH = impl::USE_LOOP_THRESHOLD_FLAG >
double test_insert_throughput(GlobalAddress<GlobalHashMap<K,V>> ha) {
  double t = Grappa::walltime();

  forall<TH>(0, FLAGS_nelems, [ha](int64_t i){
    auto k = next_random<long>() % FLAGS_max_key;
    if (choose_random(FLAGS_fraction_lookups)) {
      K v;
      bool found = ha->lookup(k, &v);
    } else {
      ha->insert(k, 2*k);
    }
  });
  
  t = Grappa::walltime() - t;
  return t;
}

void test_correctness() {
  LOG(INFO) << "Testing correctness...";
  auto ha = GlobalHashMap<long,long>::create(FLAGS_global_hash_size);
  for (int i=0; i<10; i++) {
    ha->insert(i, 42);
  }
  for (int i=0; i<10; i++) {
    long val;
    BOOST_CHECK_EQUAL(ha->lookup(i, &val), true);
    BOOST_CHECK_EQUAL(val, 42);
  }
  
  forall(10, FLAGS_nelems-10, [ha](int64_t i){
    ha->insert(i, 42);
  });

  forall(ha, [](long key, long& val){
    BOOST_CHECK_EQUAL(val, 42);
  });
  
  ha->destroy();
}

void test_set_correctness() {
  LOG(INFO) << "Testing correctness of GlobalHashSet...";
  auto sa = GlobalHashSet<long>::create(FLAGS_global_hash_size);
  
  for (int i=0; i<10; i++) {
    sa->insert(42+i);
  }
  for (int i=0; i<10; i++) {
    BOOST_CHECK_EQUAL(sa->lookup(42+i), true);
  }

  on_all_cores([sa]{
    for (int i=10; i<20; i++) {
      sa->insert(42+i);
    }
  });

  sa->forall_keys([](long& k) { BOOST_CHECK(k < 42+20 && k >= 42); });

  BOOST_CHECK_EQUAL(sa->size(), 20);
  sa->destroy();
}

double test_set_insert_throughput() {
  auto sa = GlobalHashSet<long>::create(FLAGS_global_hash_size);
  
  double t = walltime();
  
  forall(0, FLAGS_nelems, [sa](int64_t i){
    auto k = next_random<long>() % FLAGS_max_key;
    if (choose_random(FLAGS_fraction_lookups)) {
      sa->lookup(k);
    } else {
      if (FLAGS_insert_async) {
        default_gce().enroll();
        auto origin = mycore();
        sa->insert_async(k, [origin]{ default_gce().send_completion(origin); });
      } else {
        sa->insert(k);
      }
    }
  });
  if (FLAGS_insert_async) sa->sync_all_cores();

  t = walltime() - t;
  
  VLOG(1) << "set_size: " << sa->size();
  sa->destroy();
  return t;
}

BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa::init( GRAPPA_TEST_ARGS );
  Grappa::run([]{
    if (FLAGS_map_perf) {
      auto ha = GlobalHashMap<long,long>::create(FLAGS_global_hash_size);
    
      for (int i=0; i<FLAGS_ntrials; i++) {
        trial_time += test_insert_throughput<Exp::INSERT>(ha);
        ha->clear();
      }
      
      ha->destroy();
    
    } else if (FLAGS_set_perf) { 
      for (int i=0; i<FLAGS_ntrials; i++) {
        trial_time += test_set_insert_throughput();
      }
    } else {
      test_correctness();
      test_set_correctness();
    }
  
    Metrics::merge_and_print();
  });
  Grappa::finalize();
}
BOOST_AUTO_TEST_SUITE_END();
