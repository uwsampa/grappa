// graph500/grappa/
// XXX shouldn't have to include this first: common.h and oned_csr.h have cyclic dependency
#pragma once

#include <Grappa.hpp>
#include <graph/Graph.hpp>

#include <iostream>

using vindex = int;

struct PagerankVertex : public Grappa::Vertex {
  struct Data {
    double * weights;
    double v[2];
  };
  
  PagerankVertex(): Vertex() { vertex_data = new Data(); }
  
  Data* operator->() { return reinterpret_cast<Data*>(vertex_data); }
};

void spmv_mult(GlobalAddress<Grappa::Graph<PagerankVertex>> g, vindex x, vindex y);
