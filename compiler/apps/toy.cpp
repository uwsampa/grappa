#include <Grappa.hpp>
#include <GlobalVector.hpp>
#include <graph/Graph.hpp>

#ifdef __GRAPPA_CLANG__
#include <Primitive.hpp>
#endif

using namespace Grappa;
using delegate::call;

struct EdgeWeights {
  double * weights;
};
using SparseRow = Vertex<EdgeWeights>;
using SparseMatrix = Graph<SparseRow>;

DEFINE_uint64(scale, 16, "logN dimension of square matrix" );
DEFINE_uint64(nnz_factor, 16, "Approximate number of non-zeros per matrix row");

void spmv_mult(Graph symmetric* A, double global* X, double global* Y) {
  forall(A, [=](VertexID i, Vertex& v){
    auto w = v.weights;
    auto adj = v.adjacencies;
    forall<async>(0, v.nadj, [=](int64_t j){
      Y[i] += w[j] * X[adj[j]];
    });
  });
}

struct Counter {
  size_t count;
  Locale first;
};

void gups(Counter global* A, int64_t global* B, size_t N) {
  forall(0, N, [](int64_t i){
    Locale origin = here();
    int64_t prev = fetch_add(&A[B[i]].count, 1);
    if (prev == 0) {
      A[B[i]].core = origin;
    }
  });
}

int main(int argc, char* argv[]) {
  init(&argc, &argv);
  
  sizeA = 1L << FLAGS_log_array_size,
  sizeB = 1L << FLAGS_log_iterations;
  
  run([]{
    LOG(INFO) << "running";
    
    auto nv = 1L << FLAGS_scale;
    auto ne = nv * FLAGS_nnz_factor;
    
    auto A = SparseMatrix::create(
               TupleGraph::Kronecker(FLAGS_scale, ne, 111, 222)
             );
    
    auto X = as_ptr(global_alloc<int64_t>(nv));
    auto Y = as_ptr(global_alloc<int64_t>(nv));
    
    forall(B, sizeB, [](int64_t& b) {
      b = random() % sizeA;
    });
    
    LOG(INFO) << "starting timed portion";
    double start = walltime();
    
    Graph A;
    double global* X;
    double global* Y;
    
    forall(A, [=](VertexID i, Vertex& v){
      auto w = v.weights;
      auto adj = v.adjacencies;
      forall<async>(0, v.nadj, [=](int64_t j){
        Y[i] += w[j] * X[adj[j]];
      });
    });
    
    if (FLAGS_metrics) Metrics::merge_and_print();
    Metrics::merge_and_dump_to_file();
  });
  finalize();
}
