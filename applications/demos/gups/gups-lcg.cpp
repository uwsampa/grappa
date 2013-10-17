
#include <Grappa.hpp>

#include <Collective.hpp>
#include <ParallelLoop.hpp>
#include <Delegate.hpp>
#include <AsyncDelegate.hpp>
#include <GlobalAllocator.hpp>
#include <Array.hpp>

using namespace Grappa;

DEFINE_int64( iterations, 1 << 30, "Number of iterations" );
DEFINE_int64( sizeA, 1 << 10, "Size of array that GUPS increments" );

DEFINE_bool( verify, false, "Verify count of increments" );

GRAPPA_DEFINE_STAT( SimpleStatistic<double>, gups_runtime, 0.0 );
GRAPPA_DEFINE_STAT( SimpleStatistic<double>, gups_throughput, 0.0 );

const uint64_t lcgM = 6364136223846793005UL;
const uint64_t lcgB = 1442695040888963407UL;

int main( int argc, char * argv[] ) {

  init( &argc, &argv );
  
  run( [] {

      auto A = global_alloc<int64_t>(FLAGS_sizeA);
      Grappa::memset( A, 0, FLAGS_sizeA );

      double start = walltime();

      forall_global_public(0, FLAGS_iterations, [=](int64_t i){
          uint64_t offset = (lcgM * i + lcgB) % FLAGS_sizeA;
          delegate::fetch_and_add( A + offset, 1);
          //delegate::increment_async( A + offset, 1);
        });

      gups_runtime = walltime() - start;
      gups_throughput = FLAGS_iterations / gups_runtime;

      if( FLAGS_verify ) {
        static int64_t local_increment_count = 0;
        forall_localized( A, FLAGS_sizeA, [](int64_t& x) {
            local_increment_count += x;
          } );

        int64_t total_increment_count = reduce<int64_t,collective_add>(&local_increment_count);
        CHECK_EQ( total_increment_count, FLAGS_iterations ) << "Verify failed.";
      }
      
      LOG(INFO) << gups_throughput.value() << " UPS in " << gups_runtime.value() << " seconds";

      global_free(A);

    } );

  finalize();

  return 0;
}
