#include <Grappa.hpp>
#include <GlobalVector.hpp>
#include <graph/Graph.hpp>
#include <Primitive.hpp>

using namespace Grappa;

DEFINE_int64( log_array_size, 28, "Size of array that GUPS increments (log2)" );
DEFINE_int64( log_iterations, 20, "Iterations (log2)" );

static int64_t sizeA, sizeB;

DEFINE_bool( metrics, false, "Dump metrics");

GRAPPA_DEFINE_METRIC( SimpleMetric<double>, gups_runtime, 0.0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<double>, gups_throughput, 0.0 );

int main(int argc, char* argv[]) {
  init(&argc, &argv);
  
  sizeA = 1L << FLAGS_log_array_size,
  sizeB = 1L << FLAGS_log_iterations;
  
  run([]{
    LOG(INFO) << "running";
    
    int64_t global* A = global_alloc<int64_t>(sizeA);
    Grappa::memset( gaddr(A), 0, sizeA );

    int64_t global* B = global_alloc<int64_t>(sizeB);
    
    forall( gaddr(B), sizeB, [](int64_t& b) {
      b = random() % sizeA;
    });
    
    LOG(INFO) << "starting timed portion";
    double start = walltime();
    
    forall(gaddr(B), sizeB, [=](int64_t& b){
      // A[B[i]]++;
      A[b]++;
    });
    
    gups_runtime = walltime() - start;
    gups_throughput = sizeB / gups_runtime;

    LOG(INFO) << gups_throughput.value() << " UPS in " << gups_runtime.value() << " seconds";

    global_free(gaddr(B));
    global_free(gaddr(A));
    
    if (FLAGS_metrics) Metrics::merge_and_print();
      
  });
  finalize();
}
