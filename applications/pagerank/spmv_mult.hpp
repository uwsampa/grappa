// graph500/grappa/
// XXX shouldn't have to include this first: common.h and oned_csr.h have cyclic dependency
#include "../graph500/grappa/common.h"
#include "../graph500/grappa/oned_csr.h"
#include "../graph500/grappa/graph.hpp"

#include <Grappa.hpp>

#include <iostream>

struct weighted_csr_graph {
  GlobalAddress<int64_t> xoff, xadj, xadjstore;
  GlobalAddress<double> adjweight;
  int64_t nv;
  int64_t nadj;

  weighted_csr_graph() {}
  weighted_csr_graph( csr_graph copied ) 
    : xoff( copied.xoff )
    , xadj( copied.xadj )
    , xadjstore( copied.xadjstore )
    , nv( copied.nv )
    , nadj( copied.nadj ) {}
};

typedef int vindex;
struct element_pair {
  double vp[2];
};
struct vector {
  GlobalAddress<element_pair> a;
  uint64_t length;
};

struct PagerankVertex : public Vertex {
  struct Data {
    double * weights;
    double v[2];
  };
  
  PagerankVertex(): Vertex() { vertex_data = new Data(); }
  
  Data* operator->() { return reinterpret_cast<Data*>(vertex_data); }
};

void spmv_mult( weighted_csr_graph A, vector v, vindex x, vindex y );

void spmv_mult(GlobalAddress<Graph<PagerankVertex>> g);

void matrix_out( weighted_csr_graph * g, std::ostream& o, bool dense );
void R_matrix_out( weighted_csr_graph * g, std::ostream& o);
void vector_out( vector * v, std::ostream& o );
void R_vector_out( vector * v, std::ostream& o );
