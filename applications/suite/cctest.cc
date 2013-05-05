#include "defs.h"
int main() {
  graph G;
  G.numEdges = 4;
  G.numVertices = 5;
  graphint sV[8] = {0,0,1,2,3,3,4,4};
  G.startVertex = sV;
  graphint eV[8] = {1,4,0,2,3,4,3,0}; 
  G.endVertex = eV;
  G.intWeight = NULL;
  graphint eS[6] = {0, 2, 3, 4, 6, 8};
  G.edgeStart = eS;
  G.marks = new graphint[5];
  G.map_size = 0;

  int count = connectedComponents(&G);
  fprintf(stderr, "%d components\n", count);
}
