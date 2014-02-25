#include <Grappa.hpp>
#include <GlobalVector.hpp>
#include <graph/Graph.hpp>
#include <Primitive.hpp>

using namespace Grappa;

DEFINE_int64( sizeA, 1 << 28, "Size of array that GUPS increments" );
DEFINE_int64( sizeB, 1 << 20, "Number of iterations" );
DEFINE_bool( metrics, false, "Dump metrics");

GRAPPA_DEFINE_METRIC( SimpleMetric<double>, gups_runtime, 0.0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<double>, gups_throughput, 0.0 );

int main(int argc, char* argv[]) {
  init(&argc, &argv);
  run([]{
    LOG(INFO) << "running";
    
    int64_t global* A = global_alloc<int64_t>(FLAGS_sizeA);
    Grappa::memset( gaddr(A), 0, FLAGS_sizeA );

    int64_t global* B = global_alloc<int64_t>(FLAGS_sizeB);
    
    forall( gaddr(B), FLAGS_sizeB, [](int64_t& b) {
      b = random() % FLAGS_sizeA;
    });
    
    LOG(INFO) << "starting timed portion";
    double start = walltime();
    
    forall(gaddr(B), FLAGS_sizeB, [=](int64_t& b){
      // A[B[i]]++;
      A[b]++;
    });
    
    gups_runtime = walltime() - start;
    gups_throughput = FLAGS_sizeB / gups_runtime;

    LOG(INFO) << gups_throughput.value() << " UPS in " << gups_runtime.value() << " seconds";

    global_free(gaddr(B));
    global_free(gaddr(A));
    
    if (FLAGS_metrics) Metrics::merge_and_print();
      
  });
  finalize();
}
