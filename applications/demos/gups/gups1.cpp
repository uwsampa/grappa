// make -j TARGET=gups1.exe mpi_run GARGS=" --sizeB=$(( 1 << 28 )) --loop_threshold=1024" PPN=8 NNODE=12

#include <Grappa.hpp>

#include <Collective.hpp>
#include <ParallelLoop.hpp>
#include <Delegate.hpp>
#include <AsyncDelegate.hpp>
#include <GlobalAllocator.hpp>
#include <Array.hpp>

using namespace Grappa;

DEFINE_int64( sizeA, 1 << 30, "Size of array that GUPS increments" );
DEFINE_int64( sizeB, 1 << 20, "Number of iterations" );

GRAPPA_DEFINE_STAT( SimpleStatistic<double>, gups_runtime, 0.0 );
GRAPPA_DEFINE_STAT( SimpleStatistic<double>, gups_throughput, 0.0 );

int main( int argc, char * argv[] ) {
  init(&argc, &argv);
  run([]{
    auto A = global_alloc<int64_t>(FLAGS_sizeA);
    Grappa::memset( A, 0, FLAGS_sizeA );
    
    auto B = global_alloc<int64_t>(FLAGS_sizeB);
    forall( B, FLAGS_sizeB, []( int64_t i, int64_t& b ) {
      b = random() % FLAGS_sizeA;
    });
    
    double start = walltime();
    
    forall<unbound>(0, FLAGS_sizeB, [=](int64_t i){
      delegate::fetch_and_add( A + delegate::read(B+i), 1);
    });
    
    gups_runtime = walltime() - start;
    gups_throughput = FLAGS_sizeB / gups_runtime;

    LOG(INFO) << gups_throughput.value() << " UPS in " << gups_runtime.value() << " seconds";
    Statistics::merge_and_dump_to_file();
    
    global_free(B);
    global_free(A);

  });
  finalize();
  return 0;
}
