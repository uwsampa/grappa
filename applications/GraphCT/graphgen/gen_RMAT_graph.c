#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
// #include <sys/resource.h>
#include <unistd.h>
#include <assert.h>

#ifdef __MTA__
#include <sys/mta_task.h>
#include <machine/runtime.h>
#else
#include <inttypes.h>
#endif

#include "globals.h"
#include "defs.h"
#include "xmt-luc.h"


int main(int argc, char **argv)
{ 
	int64_t i, NV, NE;

	/* tuple for Scalable Data Generation  */
	graphSDG SDGdata;

	/* The graph data structure for this benchmark - see defs.h */
	graph *G;

	/* Temp vars */
	double time, timer();
	int64_t scale, edgefactor;
	char *filename;

#ifdef __MTA__
	mta_suspend_event_logging();
#endif
	// setrlimit(16); 

	if (argc != 4) {
		fprintf(stderr, "./gen_RMAT_graph <SCALE> <edge factor> <filename>\n");
		exit(-1);
	}
	scale = SCALE = atoi(argv[1]);
	edgefactor = atoi(argv[2]);
	filename = argv[3];

	xmt_luc_io_init();

	/*------------------------------------------------------------------------- */
	/*       Preamble -- Untimed                                                */
	/*------------------------------------------------------------------------- */

	/* User Interface: Configurable parameters, and global program control. */
	printf("GraphCT - Tools & Kernels for Massive Graph Analysis:\n");
	printf("Running...\n\n");
	fflush (stdout);

	getUserParameters(scale, edgefactor);

	printf("Problem Scale: %ld\n", SCALE);
	printf("No. of vertices - %ld\n", numVertices);

	/*------------------------------------------------------------------------- */
	/*       Scalable Data Generator                                            */
	/*------------------------------------------------------------------------- */

	printf("\nScalable Data Generator - genScalData() beginning execution...\n");
	fflush (stdout);
	time = timer();
	genScalData(&SDGdata, 0.55, 0.1, 0.1, 0.25);
	time  = timer() - time;

	printf("Time taken for Scalable Data Generation is %9.6lf sec.\n", time);

	/*------------------------------------------------------------------------- */
	/* Kernel 1 - Graph Construction.                                           */
	/*------------------------------------------------------------------------- */

	/* From the input edges, construct the graph 'G'.  */
	printf("\ncomputeGraph() beginning execution...\n");
	fflush (stdout);

	time = timer();

	/* Memory allocated for the graph data structure */
	{
		graph Gdirected;
		computeGraph (&Gdirected, &SDGdata);
		G = makeUndirected (&Gdirected);
		free_graph (&Gdirected);
	}
	time = timer() - time;
	free_edgelist (&SDGdata);

	printf("\tcomputeGraph() completed execution.\n");
	printf("Time taken is %9.6lf sec.\n", time);

	/*------------------------------------------------------------------------- */
	/* Write the graph to disk                                                  */
	/*------------------------------------------------------------------------- */

	NV = G->numVertices;
	NE = G->numEdges;

	int64_t arraySize = 3 + NV + NE + NE;
	uint32_t *output = xmalloc (arraySize * sizeof(int32_t));

	output[0] = (uint32_t) NE;
	output[1] = (uint32_t) NV;
	output[2] = 0;

MTA("mta assert nodep")
	for (i = 0; i < NV; i++)
	{
		output[3 + i] = (uint32_t) G->edgeStart[i];
	}

MTA("mta assert nodep")
	for (i = 0; i < NE; i++)
	{
		output[3 + NV + i] = (uint32_t) G->endVertex[i];
	}

MTA("mta assert nodep")
	for (i = 0; i < NE; i++)
	{
		output[3 + NV + NE + i] = (uint32_t) G->intWeight[i];
	}

	printf("\nWriting graph data to disk...\n");
	fflush(stdout);
	xmt_luc_snapout (filename, output, arraySize*sizeof(int32_t));
	printf("done.\n");

	free(output);
	free_graph (G);
	return 0;
}

