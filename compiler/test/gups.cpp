// make -j TARGET=gups1.exe mpi_run GARGS=" --sizeB=$(( 1 << 28 )) --loop_threshold=1024" PPN=8 NNODE=12
#define BOOST_TEST_MODULE gups
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <Grappa.hpp>
#include <Primitive.hpp>

BOOST_AUTO_TEST_SUITE( BOOST_TEST_MODULE );

using namespace Grappa;

DEFINE_int64( log_array_size, 28, "Size of array that GUPS increments (log2)" );
DEFINE_int64( log_iterations, 20, "Iterations (log2)" );

static int64_t sizeA, sizeB;

GRAPPA_DEFINE_METRIC( SimpleMetric<double>, gups_runtime, 0.0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<double>, gups_throughput, 0.0 );

BOOST_AUTO_TEST_CASE( test1 ) {
  init( GRAPPA_TEST_ARGS );
  
  sizeA = 1L << FLAGS_log_array_size,
  sizeB = 1L << FLAGS_log_iterations;
  
  run([]{
    
    int64_t global* A = global_alloc<int64_t>(sizeA);
    Grappa::memset( gaddr(A), 0, sizeA );
    
    int64_t global* B = global_alloc<int64_t>(sizeB);
    
    forall( gaddr(B), sizeB, [](int64_t& b) {
      b = random() % sizeA;
    });
    
    double start = walltime();
    
    forall(0, sizeB, [=](int64_t i){
      A[B[i]]++;
    });
    
    gups_runtime = walltime() - start;
    gups_throughput = sizeB / gups_runtime;
    
    LOG(INFO) << gups_throughput.value() << " UPS in " << gups_runtime.value() << " seconds";
    
    global_free(gaddr(B));
    global_free(gaddr(A));
    
  });
  
  finalize();
}

BOOST_AUTO_TEST_SUITE_END();
