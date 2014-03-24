#include <Grappa.hpp>
#include <GlobalVector.hpp>
#include <graph/Graph.hpp>

#ifdef __GRAPPA_CLANG__
#include <Primitive.hpp>
#endif

using namespace Grappa;
using delegate::call;

DEFINE_int64( log_array_size, 28, "Size of array that GUPS increments (log2)" );
DEFINE_int64( log_iterations, 20, "Iterations (log2)" );

/// size of index array
int64_t sizeA;
/// size of target array
int64_t sizeB;

GlobalCompletionEvent phaser;

DEFINE_bool( metrics, false, "Dump metrics");

GRAPPA_DEFINE_METRIC( SimpleMetric<double>, gups_runtime, 0.0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<double>, gups_throughput, 0.0 );

struct Counter {
  size_t count;
  int64_t winner;
};

#ifdef __GRAPPA_CLANG__
void hops(Counter global* A, int64_t global* B, size_t N) {
  forall<&phaser>(0, N, [=](int64_t i){
    auto a = &A[B[i]];
    auto prev = __sync_fetch_and_add(&a->count, 1);
    if (prev == 0) {
      a->winner = i;
    }
  });
}
#else
void hops(GlobalAddress<Counter> A, GlobalAddress<int64_t> B, size_t N) {
  forall<&phaser>(0, N, [=](int64_t i){
    Core origin = mycore();
    phaser.enroll();
    call<async,nullptr>(B+i, [=](int64_t& b){
      call<async,nullptr>(A+b, [=](Counter& a){
        auto prev = __sync_fetch_and_add(&a.count, 1);
        if (prev == 0) {
          a.winner = i;
        }
        phaser.send_completion(origin);
      });
    });
  });
}
#endif

int main(int argc, char* argv[]) {
  init(&argc, &argv);
  
  sizeA = 1L << FLAGS_log_array_size,
  sizeB = 1L << FLAGS_log_iterations;
  
  run([]{
    LOG(INFO) << "running";
    
    auto A = global_alloc<Counter>(sizeA);
    forall(A, sizeA, [](Counter& c){ c.count = 0; c.winner = -1; });
    
    auto B = global_alloc<int64_t>(sizeB);
    
    forall(B, sizeB, [](int64_t& b) {
      b = random() % sizeA;
    });
    
    GRAPPA_TIME_REGION(gups_runtime) {
      hops(A, B, sizeB);
    }
    gups_throughput = sizeB / gups_runtime;

    LOG(INFO) << gups_throughput.value() << " UPS in " << gups_runtime.value() << " seconds";

    global_free(B);
    global_free(A);
    
    if (FLAGS_metrics) Metrics::merge_and_print();
    Metrics::merge_and_dump_to_file();
  });
  finalize();
}
