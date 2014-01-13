// Originally from GraphCT, version 0.6.0
// License: GeorgiaTech

#include "GlobalAllocator.hpp"
#include "defs.hpp"

void free_graph(graph * g) {
  Grappa::global_free(g->startVertex);
  Grappa::global_free(g->endVertex);
  Grappa::global_free(g->edgeStart);
  Grappa::global_free(g->intWeight);
  Grappa::global_free(g->marks);
}

void alloc_graph(graph * g, graphint NV, graphint NE) {
  g->numEdges    = NE;
  g->numVertices = NV;
  g->startVertex = Grappa::global_alloc<graphint>(NE);
  g->endVertex = Grappa::global_alloc<graphint>(NE);
  g->edgeStart = Grappa::global_alloc<graphint>(NV+2);
  g->intWeight = Grappa::global_alloc<graphint>(NE);
  g->marks = Grappa::global_alloc<color_t>(NV);
}

void alloc_edgelist(graphedges * g, graphint NE) {  
  g->numEdges    = NE;
  g->startVertex = Grappa::global_alloc<graphint>(NE);
  g->endVertex   = Grappa::global_alloc<graphint>(NE);
  g->intWeight   = Grappa::global_alloc<graphint>(NE);
}

void free_edgelist(graphedges * g) {
  Grappa::global_free(g->startVertex);
  Grappa::global_free(g->endVertex);
  Grappa::global_free(g->intWeight);
}
