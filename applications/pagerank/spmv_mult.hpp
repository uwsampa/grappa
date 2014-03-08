// graph500/grappa/
// XXX shouldn't have to include this first: common.h and oned_csr.h have cyclic dependency
#pragma once

#include <Grappa.hpp>
#include <graph/Graph.hpp>

#include <iostream>

using vindex = int;

struct PagerankData {
  double * weights;
  double v[2];
};
using PagerankVertex = Grappa::Vertex<PagerankData>;

void spmv_mult(GlobalAddress<Grappa::Graph<PagerankVertex>> g, vindex x, vindex y);
