// make -j TARGET=gups1.exe mpi_run GARGS=" --size_b=$(( 1 << 28 )) --loop_threshold=1024" PPN=8 NNODE=12
#define BOOST_TEST_MODULE performance
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <Grappa.hpp>
#include <Primitive.hpp>

BOOST_AUTO_TEST_SUITE( BOOST_TEST_MODULE );

using namespace Grappa;

DEFINE_int64( size_a, 1L << 24, "Size of target array." );
DEFINE_int64( size_b, 1L << 20, "Size of random array." );
DEFINE_int64( iterations, 3, "Number of iterations to average over" );

GRAPPA_DEFINE_METRIC( SummarizingMetric<double>, gups_global_time, 0.0 );
GRAPPA_DEFINE_METRIC( SummarizingMetric<double>, auto_gups_global_time, 0.0 );
GRAPPA_DEFINE_METRIC( SummarizingMetric<double>, gups_local_time, 0.0 );
GRAPPA_DEFINE_METRIC( SummarizingMetric<double>, auto_gups_local_time, 0.0 );
GRAPPA_DEFINE_METRIC( SummarizingMetric<double>, async_gups_local_time, 0.0 );

BOOST_AUTO_TEST_CASE( test1 ) {
  init( GRAPPA_TEST_ARGS );
  run([]{
    
    auto gA = global_alloc<int64_t>(FLAGS_size_a);
    int64_t global* A = gA;
    
    auto gB = global_alloc<int64_t>(FLAGS_size_b);
    int64_t global* B = gB;

    Grappa::memset( gA, 0, FLAGS_size_a );
    
    forall( gB, FLAGS_size_b, [](int64_t& b) {
      b = random() % FLAGS_size_a;
    });
    
    LOG(INFO) << "gups_global";
    for (int64_t i=0; i < FLAGS_iterations; i++) {
      GRAPPA_TIMER(gups_global_time) {
        
        forall(0, FLAGS_size_b, [=](int64_t i){
          delegate::fetch_and_add(gA + delegate::read(gB+i), 1);
        });
        
      }
    }
    
    LOG(INFO) << "auto_gups_global";
    for (int64_t i=0; i < FLAGS_iterations; i++) {
      GRAPPA_TIMER(auto_gups_global_time) {
        
        forall(0, FLAGS_size_b, [=](int64_t i){
          A[B[i]]++;
        });
        
      }
    }

    LOG(INFO) << "gups_local";
    for (int64_t i=0; i < FLAGS_iterations; i++) {
      GRAPPA_TIMER(gups_local_time) {
        
        forall(gB, FLAGS_size_b, [=](int64_t i, int64_t& b){
          delegate::fetch_and_add(gA+delegate::read(gB+i), 1);
        });
        
      }
    }
    
    LOG(INFO) << "auto_gups_local";
    for (int64_t i=0; i < FLAGS_iterations; i++) {
      GRAPPA_TIMER(auto_gups_local_time) {
        
        forall(gB, FLAGS_size_b, [=](int64_t i, int64_t& b){
          A[B[i]]++;
        });
        
      }
    }
    
    LOG(INFO) << "async_gups_local";
    for (int64_t i=0; i < FLAGS_iterations; i++) {
      GRAPPA_TIMER(async_gups_local_time) {
        
        forall(gB, FLAGS_size_b, [=](int64_t& b){
          delegate::increment<async>(gA+b, 1);
        });
        
      }
    }
    
    global_free(gaddr(B));
    global_free(gaddr(A));
    
    LOG(INFO) << "\n  gups_global_time:      " << (double)gups_global_time
              << "\n  auto_gups_global_time: " << (double)auto_gups_global_time
              << "\n  gups_local_time:       " << (double)gups_local_time
              << "\n  auto_gups_local_time:  " << (double)auto_gups_local_time
              << "\n  async_gups_local_time: " << (double)async_gups_local_time;
    Metrics::merge_and_dump_to_file();
  });
  
  finalize();
}

BOOST_AUTO_TEST_SUITE_END();
