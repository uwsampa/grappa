#include <Grappa.hpp>
#include <graph/Graph.hpp>

using namespace Grappa;

// file specifications
DEFINE_string(input_path, "", "Path to graph source file");
DEFINE_string(input_format, "bintsv4", "Format of graph source file");
DEFINE_string(output_path, "", "Path to graph destination file");
DEFINE_string(output_format, "bintsv4", "Format of graph destination file");

// settings for generator
DEFINE_uint64( nnz_factor, 16, "Approximate number of non-zeros per matrix row" );
DEFINE_uint64( scale, 16, "logN dimension of square matrix" );


int main(int argc, char* argv[]) {
  Grappa::init(&argc, &argv);
  Grappa::run([]{
    TupleGraph tg;
    double t = walltime();

    if( FLAGS_input_path.empty() ) {
      uint64_t N = (1L<<FLAGS_scale);
      uint64_t desired_nnz = FLAGS_nnz_factor * N;
      long userseed = 0xDECAFBAD; // from (prng.c: default seed)
      tg = TupleGraph::Kronecker(FLAGS_scale, desired_nnz, userseed, userseed);
    } else {
      // load from file
      tg = TupleGraph::Load( FLAGS_input_path, FLAGS_input_format );
    }

    t = walltime() - t;
    LOG(INFO) << "make_graph: " << t;

    if( !FLAGS_output_path.empty() ) {
      tg.save( FLAGS_output_path, FLAGS_output_format );
    }
    
  });
  Grappa::finalize();
}
