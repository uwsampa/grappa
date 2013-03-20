#include <stdlib.h>
#include <stdio.h>
#include <sys/mta_task.h>
#include <sys/time.h>
// #include <sys/resource.h>
#include <unistd.h>
#include <machine/runtime.h>
#include "globals.h"
#include "defs.h"

int main(int argc, char **argv)
{ 
  int i, maxI, Vs, NV;

/* tuple for Scalable Data Generation  */
  graphSDG *SDGdata;
  
/* The graph data structure for this benchmark - see defs.h */
  graph *G;
  
/* Kernel 2 */
  int estEdges, numMarkedEdges, *markedEdges;

/* Kernel 4 */
  double maxBC, *BC;

/* Temp vars */
  double time, timeBC, timer();
  int scale, nprocs;

  FILE* outfp;
  outfp = fopen("results.txt", "a");
#ifdef __MTA__
  mta_suspend_event_logging();
#endif
  // setrlimit(16); 

  if (argc != 2) {
      fprintf(stderr, "./ssca2 <SCALE>\n");
      exit(-1);
  }
  scale = SCALE = atoi(argv[1]);
  
/*------------------------------------------------------------------------- */
/*       Preamble -- Untimed                                                */
/*------------------------------------------------------------------------- */
   
/* User Interface: Configurable parameters, and global program control. */
  printf("\nSSCA Graph Analysis Executable Specification:");
  printf("\nRunning...\n\n");
 
  getUserParameters(scale);

  printf("Problem Scale: %d\n", SCALE);
  printf("No. of vertices - %d\n", numVertices);
  fprintf(outfp, "No. of vertices - %d\n", numVertices);

/*------------------------------------------------------------------------- */
/*       Scalable Data Generator                                            */
/*------------------------------------------------------------------------- */

  printf("\nScalable Data Generator - genScalData() beginning execution...\n");
  time = timer();
  SDGdata  = (graphSDG*) malloc(sizeof(graphSDG));
  genScalData(SDGdata);
  /* gen1DTorus(SDGdata); */

  time  = timer() - time;
  
  printf("\nTime taken for Scalable Data Generation is %9.6lf sec.\n", time);
  fprintf(outfp, "\nScalable Data Generation - %9.6lf sec.\n", time);

/*------------------------------------------------------------------------- */
/* Kernel 1 - Graph Construction.                                           */
/*------------------------------------------------------------------------- */

/* From the input edges, construct the graph 'G'.  */
  printf("\nKernel 1 - computeGraph() beginning execution...\n");

  time = timer();

/* Memory allocated for the graph data structure */
  G = (graph *) malloc(sizeof(graph));

  computeGraph(G, SDGdata);
 
  time = timer() - time;
  
  printf("\n\tcomputeGraph() completed execution.\n");
  printf("\nTime taken for kernel 1 is %9.6lf sec.\n", time);
  fprintf(outfp, "\nKernel 1 - %9.6lf sec.\n", time);


/*------------------------------------------------------------------------- */
/* Kernel 2 - Find max weight edges                                         */
/*------------------------------------------------------------------------- */

  printf("\nKernel 2 - getStartLists() beginning execution...\n");

  time = timer();

/* Allocate memory for edge labels                                        */
  estEdges    = 2.0 * (G->numEdges / maxWeight);
  markedEdges = (int *) malloc(estEdges * sizeof(int));

  getStartLists(G, &numMarkedEdges, markedEdges);

  time  = timer() - time;

  printf("\n\tgetStartLists() completed execution.\n");
  printf("\nTime taken for kernel 2 is %9.6lf sec.\n\n", time);
  fprintf(outfp, "\nKernel 2 - %9.6lf sec.\n", time);

/*------------------------------------------------------------------------- */
/* Kernel 3 - Graph Extraction                                              */
/*------------------------------------------------------------------------- */

  printf("\nKernel 3 - findSubGraphs() beginning execution...\n");

  time = timer();

  findSubGraphs(numMarkedEdges, markedEdges, G);

  time  = timer() - time;

  printf("\n\tfindSubGraphs() completed execution.\n");
  printf("\nTime taken for kernel 3 is %9.6lf sec.\n\n", time);
  fprintf(outfp, "\nKernel 3 - %9.6lf sec.\n", time);

/*------------------------------------------------------------------------- */
/* Kernel 4 - Betweeness Centrality                                         */
/*------------------------------------------------------------------------- */

  printf("\nKernel 4 - centrality() beginning execution...\n");

  Vs = 1 << K4approx;
  NV = G->numVertices;
  BC = (double *) malloc(NV * sizeof(double));

  //setrlimit(nprocs);
  timeBC = centrality(G, BC, Vs);

  maxI  = 0;
  maxBC = BC[0];
  for (i = 1; i < NV; i++) if (BC[i] > maxBC) {maxBC = BC[i]; maxI = i;}

  printf("\n\tcentrality() completed execution.\n");
  printf("\nTime taken for kernel 4 is %9.6lf sec.\n", timeBC);
  printf("\nVertex %d has maximum BC value of %9.6lf.\n", maxI, maxBC);
  printf("\nTEPS = %9.6lf\n\n", 7 * NV * Vs / timeBC);
  fprintf(outfp, "\nKernel 4 - %9.6lf sec.\n", timeBC);
  fprintf(outfp, "\nTEPS = %9.6lf\n\n", 7 * NV * Vs / timeBC);

  fclose(outfp);

  free(SDGdata);
  free(BC);
  free(markedEdges);
  free(G->startVertex);
  free(G->endVertex);
  free(G->intWeight);
  free(G->edgeStart);
  free(G);  
  return 0;
}
