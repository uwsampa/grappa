// Originally from GraphCT, version 0.6.0
// License: GeorgiaTech

#include "GlobalAllocator.hpp"
#include "defs.hpp"

void free_graph(graph * g) {
  Grappa_free(g->startVertex);
  Grappa_free(g->endVertex);
  Grappa_free(g->edgeStart);
  Grappa_free(g->intWeight);
  Grappa_free(g->marks);
}

void alloc_graph(graph * g, graphint NV, graphint NE) {
  g->numEdges    = NE;
  g->numVertices = NV;
  g->startVertex = Grappa_typed_malloc<graphint>(NE);
  g->endVertex = Grappa_typed_malloc<graphint>(NE);
  g->edgeStart = Grappa_typed_malloc<graphint>(NV+2);
  g->intWeight = Grappa_typed_malloc<graphint>(NE);
  g->marks = Grappa_typed_malloc<color_t>(NV);
}

void alloc_edgelist(graphedges * g, graphint NE) {  
  g->numEdges    = NE;
  g->startVertex = Grappa_typed_malloc<graphint>(NE);
  g->endVertex   = Grappa_typed_malloc<graphint>(NE);
  g->intWeight   = Grappa_typed_malloc<graphint>(NE);
}

void free_edgelist(graphedges * g) {
  Grappa_free(g->startVertex);
  Grappa_free(g->endVertex);
  Grappa_free(g->intWeight);
}
