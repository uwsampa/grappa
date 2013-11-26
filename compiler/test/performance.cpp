// make -j TARGET=gups1.exe mpi_run GARGS=" --size_b=$(( 1 << 28 )) --loop_threshold=1024" PPN=8 NNODE=12

#include <Primitive.hpp>
#include <Grappa.hpp>
#include <Collective.hpp>
#include <ParallelLoop.hpp>
#include <Delegate.hpp>
#include <AsyncDelegate.hpp>
#include <GlobalAllocator.hpp>
#include <Array.hpp>

using namespace Grappa;

DEFINE_int64( size_a, 1 << 30, "Size of target array." );
DEFINE_int64( size_b, 1 << 20, "Size of random array." );
DEFINE_int64( iterations, 3, "Number of iterations to average over" );

GRAPPA_DEFINE_STAT( SummarizingStatistic<double>, gups_global_time, 0.0 );
GRAPPA_DEFINE_STAT( SummarizingStatistic<double>, auto_gups_global_time, 0.0 );
GRAPPA_DEFINE_STAT( SummarizingStatistic<double>, gups_local_time, 0.0 );
GRAPPA_DEFINE_STAT( SummarizingStatistic<double>, auto_gups_local_time, 0.0 );
GRAPPA_DEFINE_STAT( SummarizingStatistic<double>, async_gups_local_time, 0.0 );

int main( int argc, char * argv[] ) {
  init( &argc, &argv );
  
  run([]{
    
    auto gA = global_alloc<int64_t>(FLAGS_size_a);
    int64_t global* A = gA;
    
    auto gB = global_alloc<int64_t>(FLAGS_size_b);
    int64_t global* B = gB;

    Grappa::memset( gA, 0, FLAGS_size_a );
    
    forall_localized( gB, FLAGS_size_b, [](int64_t& b) {
      b = random() % FLAGS_size_a;
    });
    
    LOG(INFO) << "gups_global";
    for (int64_t i=0; i < FLAGS_iterations; i++) {
      GRAPPA_TIMER(gups_global_time) {
        
        forall_global_public(0, FLAGS_size_b, [=](int64_t i){
          delegate::fetch_and_add(gA + delegate::read(gB+i), 1);
        });
        
      }
    }
    
    LOG(INFO) << "auto_gups_global";
    for (int64_t i=0; i < FLAGS_iterations; i++) {
      GRAPPA_TIMER(auto_gups_global_time) {
        
        forall_global_public(0, FLAGS_size_b, [=](int64_t i){
          A[B[i]]++;
        });
        
      }
    }

    LOG(INFO) << "gups_local";
    for (int64_t i=0; i < FLAGS_iterations; i++) {
      GRAPPA_TIMER(gups_local_time) {
        
        forall_localized(gB, FLAGS_size_b, [=](int64_t i, int64_t& b){
          delegate::fetch_and_add(gA+delegate::read(gB+i), 1);
        });
        
      }
    }
    
    LOG(INFO) << "auto_gups_local";
    for (int64_t i=0; i < FLAGS_iterations; i++) {
      GRAPPA_TIMER(auto_gups_local_time) {
        
        forall_localized(gB, FLAGS_size_b, [=](int64_t i, int64_t& b){
          A[B[i]]++;
        });
        
      }
    }
    
    LOG(INFO) << "async_gups_local";
    for (int64_t i=0; i < FLAGS_iterations; i++) {
      GRAPPA_TIMER(async_gups_local_time) {
        
        forall_localized(gB, FLAGS_size_b, [=](int64_t& b){
          delegate::increment_async(gA+b, 1);
        });
        
      }
    }
    
    global_free(gaddr(B));
    global_free(gaddr(A));
    
//    Statistics::merge_and_print();
    LOG(INFO) << "\n  gups_global_time:      " << (double)gups_global_time
              << "\n  auto_gups_global_time: " << (double)auto_gups_global_time
              << "\n  gups_local_time:       " << (double)gups_local_time
              << "\n  auto_gups_local_time:  " << (double)auto_gups_local_time
              << "\n  async_gups_local_time: " << (double)async_gups_local_time;
    Statistics::merge_and_dump_to_file();
  });
  
  finalize();
  return 0;
}
