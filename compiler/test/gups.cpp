// make -j TARGET=gups1.exe mpi_run GARGS=" --sizeB=$(( 1 << 28 )) --loop_threshold=1024" PPN=8 NNODE=12

#include <Grappa.hpp>
#include <Primitive.hpp>

using namespace Grappa;

DEFINE_int64( sizeA, 1 << 30, "Size of array that GUPS increments" );
DEFINE_int64( sizeB, 1 << 20, "Number of iterations" );

GRAPPA_DEFINE_METRIC( SimpleMetric<double>, gups_runtime, 0.0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<double>, gups_throughput, 0.0 );

int main( int argc, char * argv[] ) {
  init( &argc, &argv );
 
  run([]{

    int64_t global* A = global_alloc<int64_t>(FLAGS_sizeA);
    Grappa::memset( gaddr(A), 0, FLAGS_sizeA );

    int64_t global* B = global_alloc<int64_t>(FLAGS_sizeB);
    forall( gaddr(B), FLAGS_sizeB, [](int64_t& b) {
      b = random() % FLAGS_sizeA;
    });

    double start = walltime();

    forall(0, FLAGS_sizeB, [=](int64_t i){
      A[B[i]]++;
    });

    gups_runtime = walltime() - start;
    gups_throughput = FLAGS_sizeB / gups_runtime;

    LOG(INFO) << gups_throughput.value() << " UPS in " << gups_runtime.value() << " seconds";

    global_free(gaddr(B));
    global_free(gaddr(A));

  });

  finalize();
  return 0;
}
