// Originally from GraphCT, version 0.6.0
// License: GeorgiaTech

#include "GlobalAllocator.hpp"
#include "defs.hpp"

void free_graph(graph * g) {
  SoftXMT_free(g->startVertex);
  SoftXMT_free(g->endVertex);
  SoftXMT_free(g->edgeStart);
  SoftXMT_free(g->intWeight);
  SoftXMT_free(g->marks);
}

void alloc_graph(graph * g, graphint NV, graphint NE) {
  g->numEdges    = NE;
  g->numVertices = NV;
  g->startVertex = SoftXMT_typed_malloc<graphint>(NE);
  g->endVertex = SoftXMT_typed_malloc<graphint>(NE);
  g->edgeStart = SoftXMT_typed_malloc<graphint>(NV+2);
  g->intWeight = SoftXMT_typed_malloc<graphint>(NE);
  g->marks = SoftXMT_typed_malloc<color_t>(NV);
}

void alloc_edgelist(graphedges * g, graphint NE) {  
  g->numEdges    = NE;
  g->startVertex = SoftXMT_typed_malloc<graphint>(NE);
  g->endVertex   = SoftXMT_typed_malloc<graphint>(NE);
  g->intWeight   = SoftXMT_typed_malloc<graphint>(NE);
}

void free_edgelist(graphedges * g) {
  SoftXMT_free(g->startVertex);
  SoftXMT_free(g->endVertex);
  SoftXMT_free(g->intWeight);
}
