#include <math.h>
#include "defs.h"
#ifdef __MTA__
#include "mta_rng.h"
#endif
int cmp_u(const void * x, const void * y) {
  double u1 = ((double *) x)[0];
  double u2 = ((double *) y)[0];
  return (u1 < u2 ? -1 : (u1 > u2 ? 1 : 0));
}
int cmp_v(const void * x, const void * y) {
  double v1 = ((double *) x)[1];
  double v2 = ((double *) y)[1];
  return (v1 < v2 ? -1 : (v1 > v2 ? 1 : 0));
}
int main(int argc, char **argv) {
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
  graphint marks[6] = {0, 0, 0, 0, 0, 0};
  G.marks = marks;
  G.map_size = 0;

  int count = connectedComponents(&G);
  fprintf(stderr, "%d components\n", count);

  int NV = atoi(argv[1]);
  int NE = atoi(argv[2]);
  G.numEdges = NE;
  G.numVertices = NV;
  G.startVertex = new graphint[NE*2+1];
  G.endVertex = new graphint[NE*2];
  G.edgeStart = new graphint[NV+1];
  G.marks = new graphint[NV];

  double * temp = new double[4*NE];
#ifdef __MTA__
  prand_int(2*NE, (int*) temp);
  for (int i = 0; i < 2*NE; i++) temp[i] = ((unsigned int*)temp)[i]%NV;
#else
  for (int i = 0; i < 2*NE; i++) temp[i] = random()%NV;
#endif
  MTA("mta assert nodep") 
  for (int i = 2*NE; i < 4*NE; i+=2) temp[i] = temp[i+1-2*NE], temp[i+1] = temp[i-2*NE];
  fprintf(stderr, "sort pass 1...\n");
  qsort(temp, 2*NE, 2*sizeof(temp[0]), cmp_v);
  for (int i = 0; i < 4*NE; i+=2)  temp[i] += double(i)/4/NE;
  fprintf(stderr, "sort pass 2...\n");
  qsort(temp, 2*NE, 2*sizeof(temp[0]), cmp_u);
  fprintf(stderr, "sort pass 2 complete\n");
  MTA("mta assert nodep")
  for (int i = 0; i < 2*NE; i++) G.startVertex[i] = temp[2*i], G.endVertex[i] = temp[2*i+1];

  G.startVertex[2*NE] = NV; /* sentinel */
  G.edgeStart[0] = 0;
  graphint v = 0;
#ifdef __MTA__
#pragma mta assert parallel
  for (int v = 0; v <=NV; v++) {
    int min = 0, max = 2*NE+1;
    int guess = (max-min)*v/NV;
    do {
      if (G.startVertex[guess] >= v) {
	if (guess == 0) break;
	if (G.startVertex[guess-1] < v) break;
	max = guess-1;
      } else {
	//	if (G.startVertex[guess+1] == v) guess = guess+1, break;
	min = guess+1;
      }
      guess = (max+min)/2;
    } while (max >= min);
    G.edgeStart[v] = guess;
  } 
#else
  for (int i = 0; i <= 2*NE; i++) {
    while (v < G.startVertex[i]) G.edgeStart[++v] = i;
  }
#endif

#if 0
  for (int i = 0; i <= NV; i++) fprintf(stderr, "%3d ", G.edgeStart[i]); fprintf(stderr, "\n");
  for (int i = 0; i < 2*NE; i++) fprintf(stderr, "%3d ", G.startVertex[i]); fprintf(stderr, "\n");
  for (int i = 0; i < 2*NE; i++) fprintf(stderr, "%3d ", G.endVertex[i]); fprintf(stderr, "\n");
  int j = 1;
  for (int i = 0; i < 2*NE; i++) {
    if (G.edgeStart[j] != i) fprintf(stderr, "    ");
    else {fprintf(stderr, "%3d ", j), j++;}
  } fprintf(stderr, "\n");
  fprintf(stderr, "G.edgeStart[%d]=%d\n", j, G.edgeStart[j]);
#endif

  count = connectedComponents(&G);
  fprintf(stderr, "%d components\n", count);
}

