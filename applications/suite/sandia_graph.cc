#include <math.h>
#include "defs.h"
#ifdef __MTA__
#include <mta_rng.h>
#include <machine/mtaops.h>
#endif
#include <sys/time.h>

void write_graph(graph *G, char * fname = NULL);
//void read_graph(graph *G, char * fname = NULL);



int min(int i, int j) {
  return i < j ? i : j;
}

// Stably sorts the n values in src.
// The values are pairs of int values.
// We sort by the 0th field (the subject).
// Returns a pointer to a vector containing the sorted data
// The original src vector may be returned or freed.
// All values must be <= largest.
// Spends O(n) space for buckets, in an effort to approximately balance
// the time spent scanning the data and the time spent maintaining the bucket vector.

#pragma mta no inline
int *radix_sort(int *src, int n,
		int largest, int keyfield) {
  int threads_per_processor = 100;
  int processors = mta_get_num_teams();
  int threads = processors*threads_per_processor;
  int space = n/threads;
  if (space < 16) space = 16;
  int spacebits = 64 - MTA_BIT_LEFT_ZEROS(space - 1); // number of bits in space
  int keybits = 64 - MTA_BIT_LEFT_ZEROS(largest); // number of bits in the largest key
  int passes = (keybits + spacebits - 1)/spacebits;
  int bits = (keybits + passes - 1)/passes; // log(radix)
  int max = 1 << bits;
  int buckets = max*threads;
  int *dst = (int *) malloc(2*n*sizeof(int));
  int *bucket = (int *) malloc((buckets + 1)*sizeof(int));
  int chunk = (n + threads - 1)/threads;
  int mask = max - 1;
  for (int pass = 0; pass < passes; pass++) {
    for (int i = 0; i <= buckets; i++)
      bucket[i] = 0;

    // collect histogram
    for (int t = 0; t < threads; t++) {
      int limit = min((t + 1)*chunk, n);
      for (int i = t*chunk; i < limit; i++) {
        int r = MTA_BIT_PACK(~mask, src[2*i + keyfield]);
        bucket[t + threads*r + 1]++;
      }
    }
    
  // compute starting location for each bucket
    for (int i = 1; i < buckets; i++)
      bucket[i] += bucket[i - 1];

    // place elements
#pragma mta assert parallel
    for (int t = 0; t < threads; t++) {
      int limit = min((t + 1)*chunk, n);
      for (int i = t*chunk; i < limit; i++) {
	int subject = src[2*i + 0];
	int object  = src[2*i + 1];
        int r = MTA_BIT_PACK(~mask, src[2*i + keyfield]);
        int j = bucket[t + threads*r]++;
        dst[2*j + 0] = subject;
        dst[2*j + 1] = object;
      }
    }

    int *tmp = src;
    src = dst;
    dst = tmp;
    mask <<= bits;
  }
  free(dst);
  free(bucket);
  return src;
}



int fast_poisson_random(double lambda, double* R, int off, int R_size) {
  static unsigned BIG_PRIME = 29637231989;
  double L = exp(-lambda);
  double p = 1.0;
  int k = 0;
  do {
    k = k+1;
    p = p * R[(off*BIG_PRIME+k)&(R_size-1)];
  } while (p > L);
  return k-1;
}

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
#define DEGREE_20_NODES (1<<5)
#define DEGREE_10_NODES (1<<15)
#define DEGREE_5_NODES (1<<25)
  int lgNV = atoi(argv[1]);
  int runs = 1; if (argc > 2) runs = atoi(argv[2]);
  if (lgNV == 0) goto slurp_graph;
  int NV = 1<<lgNV; //25 for Sandia
  fprintf(stderr, "Building Sandia graph w/ %d vertices.\n", NV);

  int *OutDegree = new int[NV];
  fprintf(stderr, "generating randoms\n");
  const int Randoms_size = 1<<20;
  double * Randoms = (double *) malloc(sizeof(double)*Randoms_size);
  prand(Randoms_size, Randoms);
  prand_int(NV, OutDegree);
  fprintf(stderr, "computing out degrees...\n");
  int deg1, deg2, deg3; deg1=deg2=deg3=0;
  for (int i = 0; i < NV; i++) {
    int edges_bound;
    int nt = OutDegree[i] & (DEGREE_5_NODES-1);
    if (nt <= DEGREE_20_NODES) {
      edges_bound = (1 << 20), deg1++;
    } else if (nt <= (DEGREE_20_NODES + DEGREE_10_NODES)) {
      edges_bound = (1 << 10), deg2++;
    }
    else {
      //edges_bound = i&1 ? 2:1;
      edges_bound = fast_poisson_random(1.5,Randoms,i,Randoms_size);
      deg3++;
    }
    OutDegree[i] = edges_bound;
  }
  fprintf(stderr, "outdegrees: %d %d %d\n", deg1, deg2, deg3);
  for (int i = 1; i < NV; i++) OutDegree[i] += OutDegree[i-1];
  int NUE = OutDegree[NV-1];//undirected edges

  // Now create edges
  fprintf(stderr, "creating %d undirected edges...\n", NUE);
  // first set the sources (& corresponding dsts of the back edges...)
  int* temp = new int[4*NUE];
  for (int j = 0; j < OutDegree[0]; j++) {
    temp[2*j] = 0;//forward src
    temp[2*NUE + 2*j+1] = 0;//backward dst 
  }
  MTA("mta assert nodep")
  for (int i = 1; i < NV; i++) {
    for (int j = OutDegree[i-1]; j < OutDegree[i]; j++) {
      temp[2*j] = i;
      temp[2*NUE +2*j+1] = i;
    }
  }
  // now create random dests (&corresponding srcs for the back edges)
  int* temp2 = new int[NUE];
  prand_int(NUE, temp2);
  for (int i = 0; i < NUE; i++)
    temp2[i] = temp2[i] & (NV - 1);
  //Create back edges:
  MTA("mta assert nodep")
  for (int i = 0; i < NUE; i++) temp[2*i+1] = temp[2*NUE+2*i] = temp2[i];
  delete temp2;
  //Edges are in place as src[0],dst[0],src[1],dst[1],...
  // with the property that src[i] = dst[i+NUE] & dst[i] = src[i+NUE]
  fprintf(stderr, "sorting edges...\n");
  fprintf(stderr, "sort pass 1...\n");
  //qsort(temp, 2*NUE, 2*sizeof(temp[0]), cmp_v);
  int* dest = radix_sort(temp, 2*NUE, NV - 1, 1);
  fprintf(stderr, "sort pass 2...\n");
  //  qsort(temp, 2*NUE, 2*sizeof(temp[0]), cmp_u);
  temp = radix_sort(dest, 2*NUE, NV - 1, 0);
  fprintf(stderr, "sort pass 2 complete\n");
  fprintf(stderr, "Defining graph...\n");

  graph G;
  G.map_size = 0;
  G.numEdges = 2*NUE;
  G.numVertices = NV;
  G.startVertex = new graphint[NUE*2+1];
  G.endVertex = new graphint[NUE*2];
  G.edgeStart = new graphint[NV+1];
  G.marks = new graphint[NV];

  MTA("mta assert nodep")
  for (int i = 0; i < 2*NUE; i++) G.startVertex[i] = temp[2*i], G.endVertex[i] = temp[2*i+1];

  G.startVertex[2*NUE] = NV; /* sentinel */
  G.edgeStart[0] = 0;
  graphint v = 0;
#ifdef __MTA__
#pragma mta assert parallel
  for (int v = 0; v <=NV; v++) {
    int min = 0, max = 2*NUE+1;
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
  for (int i = 0; i <= 2*NUE; i++) {
    while (v < G.startVertex[i]) G.edgeStart[++v] = i;
  }
#endif

#if 0
  //
  int degsums[3] = {0,0,0};
  int degcount[3] = {0,0,0};
  int degmax[3] = {0,0,0};
  int degmin[3] = {9999999999,9999999999,9999999999};
  fprintf(stderr, "total directed edges: %d\n", G.edgeStart[NV]);
  MTA("mta assert parallel")
  for (int i = 0; i < NV; i++) {
    int deg = (G.edgeStart[i+1]-G.edgeStart[i]);
    if (deg > 2000000) {
      fprintf(stderr, "G.edgeStart[%d+1] = %d, [%d] = %d\n",
	      i,G.edgeStart[i+1],i,G.edgeStart[i]);
    }
    if (deg < 100) {
      degsums[0]+=deg, degcount[0]++;
      degmax[0] = deg > degmax[0] ? deg : degmax[0];
      degmin[0] = deg < degmin[0] ? deg : degmin[0];
    }
    else if (deg < 10000) {
      degsums[1]+=deg, degcount[1]++;
      degmax[1] = deg > degmax[1] ? deg : degmax[1];
      degmin[1] = deg < degmin[1] ? deg : degmin[1];
    }
    else {
      degsums[2]+= deg, degcount[2]++;
      degmax[2] = deg > degmax[2] ? deg : degmax[2];
      degmin[2] = deg < degmin[2] ? deg : degmin[2];
    }
  }
  fprintf(stderr, "<100: %d [%d,%d] avg %d; <10k %d [%d,%d] avg %d; >10k %d [%d,%d] avg %d\n",
	  degcount[0], degmin[0], degmax[0], degsums[0]/degcount[0],
	  degcount[1], degmin[1], degmax[1], degsums[2]/degcount[1],
	  degcount[2], degmin[2], degmax[2], degsums[2]/degcount[2]);
  //

  for (int i = 0; i <= NV; i++) fprintf(stderr, "%3d ", G.edgeStart[i]); fprintf(stderr, "\n");
  for (int i = 0; i < 2*NUE; i++) fprintf(stderr, "%3d ", G.startVertex[i]); fprintf(stderr, "\n");
  for (int i = 0; i < 2*NUE; i++) fprintf(stderr, "%3d ", G.endVertex[i]); fprintf(stderr, "\n");
  int j = 1;
  for (int i = 0; i < 2*NUE; i++) {
    if (G.edgeStart[j] != i) fprintf(stderr, "    ");
    else {fprintf(stderr, "%3d ", j), j++;}
  } fprintf(stderr, "\n");
  fprintf(stderr, "G.edgeStart[%d]=%d\n", j, G.edgeStart[j]);
#endif
#if 0
  write_graph(&G);
#endif
  goto good_to_go;
slurp_graph:
  //read_graph(&G);
good_to_go:
  fprintf(stderr, "Graph is good to go...  calling CC\n");

  for (int i = 0; i < runs; i++) {
    struct timeval tp1; gettimeofday(&tp1, 0);
    int count = connectedComponents(&G);
    struct timeval tp2; gettimeofday(&tp2, 0);
    int usec = (tp2.tv_sec-tp1.tv_sec)*1000000+(tp2.tv_usec-tp1.tv_usec);
    fprintf(stderr, "%d components computed in %g secs (%d TEPS)\n", count,usec/1000000.0,2*NUE*1000000/usec);
  }
}

