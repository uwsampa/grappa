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
#endif

#include "globals.h"
#include "defs.h"


int main(int argc, char **argv)
{ 
	/* tuple for Scalable Data Generation  */
	graphSDG SDGdata;

	/* The graph data structure for this benchmark - see defs.h */
	graph *G;
	int64_t external_graph = 0;

	/* Kernel 2 */
	int64_t estEdges;
	int64_t numMarkedEdges, *markedEdges;

	int64_t *colors;
	int64_t numComp, max, maxV;

	/* Temp vars */
	double time, timer();
	int64_t scale;

	FILE *outfp;
	outfp = fopen("results.txt", "a");
#ifdef __MTA__
	mta_suspend_event_logging();
#endif
	// setrlimit(16); 

	if (argc < 2) {
		fprintf(stderr, "./GraphCT <SCALE> [input file]\n");
		exit(-1);
	}
	scale = SCALE = atoi(argv[1]);
	if (argc > 2) {
		external_graph = 1;
	}

	fprintf(outfp, "\n------------------------------------------------------------\n");
	if (external_graph) {
		fprintf(outfp, "Input filename: %s\n", argv[2]);
	}


	/*------------------------------------------------------------------------- */
	/*       Preamble -- Untimed                                                */
	/*------------------------------------------------------------------------- */

	/* User Interface: Configurable parameters, and global program control. */
	printf("GraphCT - Tools & Kernels for Massive Graph Analysis:\n");
	printf("Running...\n\n");
	fflush (stdout);

	getUserParameters(scale, 8);

	printf("Problem Scale: %ld\n", SCALE);
	printf("No. of vertices - %ld\n", numVertices);
	fprintf(outfp, "No. of vertices - %ld\n", numVertices);

	/*------------------------------------------------------------------------- */
	/*       Scalable Data Generator                                            */
	/*------------------------------------------------------------------------- */

	if (!external_graph) {
		printf("\nScalable Data Generator - genScalData() beginning execution...\n");
		fflush (stdout);
		time = timer();
		genScalData(&SDGdata, 0.55, 0.1, 0.1, 0.25);
		/* gen1DTorus(SDGdata); */

		time  = timer() - time;

		printf("Time taken for Scalable Data Generation is %9.6lf sec.\n", time);
		fprintf(outfp, "\nScalable Data Generation - %9.6lf sec.\n", time);
	}
	else
		printf("\nScalable Data Generator - Using external graph, skipping...\n");

	/*------------------------------------------------------------------------- */
	/* Kernel 1 - Graph Construction.                                           */
	/*------------------------------------------------------------------------- */

	/* From the input edges, construct the graph 'G'.  */
	printf("\nKernel 1 - computeGraph() beginning execution...\n");
	fflush (stdout);

	time = timer();

	/* Memory allocated for the graph data structure */

	if (!external_graph) {
		graph Gdirected;
		computeGraph (&Gdirected, &SDGdata);
		G = makeUndirected (&Gdirected);
		free_graph (&Gdirected);
	} else {
	    G = parse_DIMACS_graph(argv[2]);
	}

	time = timer() - time;

	printf("\tcomputeGraph() completed execution.\n");
	printf("Time taken for kernel 1 is %9.6lf sec.\n", time);
	fprintf(outfp, "\nKernel 1 - %9.6lf sec.\n", time);

	if (!external_graph)
	{
		free_edgelist (&SDGdata);
	}

	/*------------------------------------------------------------------------- */
	/* Kernel 2 - Find max weight edges                                         */
	/*------------------------------------------------------------------------- */

	if (!external_graph) {
		printf("\nKernel 2 - getStartLists() beginning execution...\n");
		fflush (stdout);

		time = timer();

		/* Allocate memory for edge labels                                        */
		estEdges    = 2.0 * (G->numEdges / maxWeight);
		markedEdges = xmalloc(estEdges * sizeof(markedEdges));

		getStartLists(G, &numMarkedEdges, markedEdges);

		time  = timer() - time;

		printf("\tgetStartLists() completed execution.\n");
		printf("Time taken for kernel 2 is %9.6lf sec.\n", time);
		fprintf(outfp, "\nKernel 2 - %9.6lf sec.\n", time);
	}


	/*------------------------------------------------------------------------- */
	/* Kernel 3 - Graph Extraction                                              */
	/*------------------------------------------------------------------------- */

	if (!external_graph) {
		printf("\nKernel 3 - findSubGraphs() beginning execution...\n");
		fflush (stdout);

		time = timer();

		findSubGraphs(numMarkedEdges, markedEdges, G);
		free(markedEdges);

		time  = timer() - time;

		printf("\tfindSubGraphs() completed execution.\n");
		printf("Time taken for kernel 3 is %9.6lf sec.\n", time);
		fprintf(outfp, "\nKernel 3 - %9.6lf sec.\n", time);
	}


	/*------------------------------------------------------------------------- */
	/* Degree Distributions                                                     */
	/*------------------------------------------------------------------------- */

	printf("\nCalculating out-degree distributions...\n");
	fflush (stdout);
	time = timer();
	calculateDegreeDistributions(G);
	time = timer() - time;
	printf("Time taken is %9.6lf sec.\n", time);


	/*------------------------------------------------------------------------- */
	/* Connected Components                                                     */
	/*------------------------------------------------------------------------- */

	colors = G->marks;
	/* necessary to set up something for modularity... */
	printf("\nFinding connected components...\n");
	fflush (stdout);
	time = timer();
	numComp = connectedComponents(G);
	time = timer() - time;
	printf("There are %ld components in the graph.\n", numComp);
	calculateComponentDistributions(G, numComp, &max, &maxV);
	printf("Time taken is %9.6lf sec.\n", time);


	/*------------------------------------------------------------------------- */
	/* Graph Diameter                                                           */
	/*------------------------------------------------------------------------- */

	printf("\nCalculating graph diameter...\n");
	fflush (stdout);
	time = timer();
	int64_t est_diameter = calculateGraphDiameter(G, 8);
	time = timer() - time;
	printf("The calculated diameter is %ld\n", est_diameter);
	printf("Time taken is %9.6lf sec.\n", time);





	fclose(outfp);

	free_graph (G);

	return 0;
}

