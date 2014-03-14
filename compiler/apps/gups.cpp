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

static int64_t sizeA, sizeB;

GlobalCompletionEvent phaser;

DEFINE_bool( metrics, false, "Dump metrics");

GRAPPA_DEFINE_METRIC( SimpleMetric<double>, gups_runtime, 0.0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<double>, gups_throughput, 0.0 );

#if defined(__GRAPPA_CLANG__) && defined(VOID_INTERFACE)
retcode_t incr(void* args, void* out) {
  auto& a = *reinterpret_cast<GlobalAddress<int64_t>*>(args);
  auto p = a.pointer();
  (*p)++;
  return 0;
}
#endif
int main(int argc, char* argv[]) {
  init(&argc, &argv);
  
  sizeA = 1L << FLAGS_log_array_size,
  sizeB = 1L << FLAGS_log_iterations;
  
  run([]{
    LOG(INFO) << "running";
    
    auto A = global_alloc<int64_t>(sizeA);
    Grappa::memset(A, 0, sizeA );

    auto B = global_alloc<int64_t>(sizeB);
    
    forall(B, sizeB, [](int64_t& b) {
      b = random() % sizeA;
    });
    
    LOG(INFO) << "starting timed portion";
    double start = walltime();
    
#ifdef __GRAPPA_CLANG__
#  if defined(MULTIHOP)
    
    auto a = as_ptr(A), b = as_ptr(B);
    forall(0, sizeB, [=](int64_t i){
      a[b[i]]++;
    });
    
#  elif defined(VOID_INTERFACE)
    forall(B, sizeB, [=](int64_t& b){
      auto in = A+b;
      grappa_on_async(in.core(), incr, &in, sizeof(in), &impl::local_gce);
    });
#  else
    auto a = as_ptr(A);
    forall(B, sizeB, [=](int64_t& b){
      a[b]++;
    });
#  endif
#else // not __GRAPPA_CLANG__
# if defined(MULTIHOP)
    
#  if defined(BLOCKING)
    forall(0, sizeB, [=](int64_t i){
      delegate::increment(A+delegate::read(B+i), 1);
    });
#  else
    
    forall<&phaser>(0, sizeB, [=](int64_t i){
      Core origin = mycore();
      phaser.enroll();
      call<async,nullptr>(B+i, [=](int64_t& b){
        call<async,nullptr>(A+b, [=](int64_t& a){
          a++;
          phaser.send_completion(origin);
        });
      });
    });
#  endif
# else
    forall(B, sizeB, [=](int64_t& b){
#  ifdef BLOCKING
      delegate::increment(A+b, 1);
#  else
      delegate::increment<async>( A + b, 1);
#  endif
    });
# endif
#endif
    
    gups_runtime = walltime() - start;
    gups_throughput = sizeB / gups_runtime;

    LOG(INFO) << gups_throughput.value() << " UPS in " << gups_runtime.value() << " seconds";

    global_free(B);
    global_free(A);
    
    if (FLAGS_metrics) Metrics::merge_and_print();
      
  });
  finalize();
}
