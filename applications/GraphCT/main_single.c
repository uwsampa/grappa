#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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
	int64_t i, j, maxI, Vs, NV;

	graph *G;

	/* Kernel 2 */
//	int64_t estEdges, numMarkedEdges, *markedEdges;

	/* Kernel 4 */
	double maxBC, *BC;

	int64_t *colors;
	int64_t numComp, max, maxV;
	double *trans;
	int64_t diameter = 0;

	/* Temp vars */
	double time, timeBC, timer();
	int64_t scale;

#ifdef __MTA__
	mta_suspend_event_logging();
#endif
	// setrlimit(16); 

	singleKernelOptions options;
	options.cmdsingle = 1;
	options.cmdscript = 0;
	parseCommandLineOptions(argc, argv, &options);

	scale = SCALE = options.scale;


	/*------------------------------------------------------------------------- */
	/*       Preamble -- Untimed                                                */
	/*------------------------------------------------------------------------- */

	/* User Interface: Configurable parameters, and global program control. */
	printf("\nGraphCT - Tools & Kernels for Massive Graph Analysis:\n");
	printf("Running...\n\n");
	fflush (stdout);

//	getUserParameters(scale, 8);

//	printf("Problem Scale: %d\n", scale);
//	printf("No. of vertices - %d\n", numVertices);


	/*------------------------------------------------------------------------- */
	/*     Graph Construction                                                   */
	/*------------------------------------------------------------------------- */

	/* From the input edges, construct the graph 'G'.  */
	printf("\nKernel 1 - computeGraph() beginning execution...\n");
	fflush (stdout);

	time = timer();

	if (options.graph_type == 1)
	{
		G = (graph *) xmalloc(sizeof(graph));
		graphio_b(G, options.infilename, 4);
		G->undirected = 1;
	}
	else if (options.graph_type == 4)
	{
		G = (graph *) xmalloc(sizeof(graph));
		graphio_el(G, options.infilename);
		G->undirected = 1;
	}
	else if (options.graph_type == 2)
	{
		G = parse_DIMACS_graph(options.infilename);
		G->undirected = 1;
	}
	else if (options.graph_type == 3)
	{
		char tmp;
		int64_t scale = 0, edgefactor = 0;
		graphSDG SDGdata;
		graph Gdirected;
		sscanf (options.infilename, "%ld%1[.-_: ]%ld", &scale, &tmp, &edgefactor);
		if (scale <= 0 || edgefactor <= 0) {
			fprintf (stderr, "Improper scale:edgefactor : %s\n",
				 options.infilename);
			abort ();
		}
		SCALE = scale;
		getUserParameters (scale, edgefactor);
		genScalData(&SDGdata, 0.55, 0.1, 0.1, 0.25);
		computeGraph (&Gdirected, &SDGdata);
		G = makeUndirected (&Gdirected);
		free_graph (&Gdirected);
		free_edgelist (&SDGdata);
	}
	else {
	  fprintf (stderr, "Unrecognized graph type.\n");
	  abort ();
	}

	graphCheck(G);

	time = timer() - time;

	printf("\tcomputeGraph() completed execution.\n");
	printf("Time taken for kernel 1 is %9.6lf sec.\n", time);


	/*------------------------------------------------------------------------- */
	/*     Run for all kernels                                                  */
	/*------------------------------------------------------------------------- */

	if (options.scale <= 0) {
		printf("\nEstimating the graph's diameter using 256 sources...\n");
		fflush(stdout);

		time = timer();
		NV = G->numVertices;
		scale = SCALE = NV >> 4;
		stats_tic ("estdiameter");
		int64_t est_diameter = calculateGraphDiameter(G, 256);
		stats_toc ();
		options.scale = est_diameter << 2;
		scale = SCALE = options.scale;
		time = timer() - time;

		printf("Estimated graph diameter: %ld\n", est_diameter);
		printf("Setting SCALE appropriately.\n");
		printf("Time taken is %9.6lf sec.\n", time);
	}
	else {
		printf("\nUser-provided diameter estimate: %ld\n", SCALE);
		printf("Setting SCALE appropriately.\n");
	}
	getUserParameters(scale, 8);

	printf("\nCalculating out-degree distributions...\n");
	fflush (stdout);
	time = timer();
	stats_tic ("degreedist");
	calculateDegreeDistributions(G);
	stats_toc ();
	time = timer() - time;
	printf("Time taken is %9.6lf sec.\n", time);


	/*------------------------------------------------------------------------- */
	/* Betweeness Centrality                                                    */
	/*------------------------------------------------------------------------- */

	if (options.kernel_id == 1) {
		Vs = options.parameter2;
		NV = G->numVertices;
		BC = (double *) xmalloc(NV * sizeof(double));
		int64_t K = 1, tmp, exact = 0;
		tmp = options.parameter1;
		if (tmp >= 0) K = tmp;
		if (Vs == G->numVertices)
			exact = 1;
		
		printf("\nkcentrality(%ld)%s beginning execution...\n", K,
				(exact? " EXACT" : ""));
		fflush (stdout);

		stats_tic ("kcentrality");
		timeBC = kcentrality(G, BC, Vs, K);
		stats_toc ();

		if (options.outfilename)
			fwrite_doubles(BC, G->numVertices, options.outfilename);
		printf("\tkcentrality() completed execution.\n");

		printf("Maximum kBC Vertices\n");
		printf("--------------------\n");
		for (j = 0; j < 10; j++) {
			maxI  = 0;
			maxBC = BC[0];
			for (i = 1; i < NV; i++) if (BC[i] > maxBC) {maxBC = BC[i]; maxI = i;}
			printf("#%2ld: %8ld - %9.6lf\n", j+1, maxI, maxBC);
			BC[maxI] = 0.0;
		}

		printf("Time taken for kernel X is %9.6lf sec.\n", timeBC);

		free(BC);
	}


	/*------------------------------------------------------------------------- */
	/* Degree Distributions                                                     */
	/*------------------------------------------------------------------------- */

	if (options.kernel_id == 3) {
		printf("\nCalculating out-degree distributions...\n");
		fflush (stdout);
		time = timer();
		calculateDegreeDistributions(G);
		time = timer() - time;
		printf("Time taken is %9.6lf sec.\n", time);
	}


	/*------------------------------------------------------------------------- */
	/* Graph Diameter                                                           */
	/*------------------------------------------------------------------------- */

	if (options.kernel_id == 8) {
		Vs = options.parameter1;
		Vs = ((double)Vs/(double)100)*NV;
		printf("\nCalculating graph diameter using %ld sources...\n", Vs);
		fflush (stdout);
		time = timer();
		stats_tic ("diameter");
		diameter = calculateGraphDiameter(G, Vs);
		stats_toc ();
		time = timer() - time;
		printf("The diameter of the graph is %ld.\n", diameter);
		printf("Time taken is %9.6lf sec.\n", time);
	}


	/*------------------------------------------------------------------------- */
	/* Connected Components                                                     */
	/*------------------------------------------------------------------------- */

	colors = G->marks;
	/* necessary to set up something for modularity... */
	if (options.kernel_id == 5) {
		printf("\nFinding connected components...\n");
		fflush (stdout);
		time = timer();
		stats_tic ("components");
		numComp = connectedComponents(G);
		stats_toc ();
		time = timer() - time;
		printf("There are %ld components in the graph.\n", numComp);
		calculateComponentDistributions(G, numComp, &max, &maxV);
		printf("Time taken is %9.6lf sec.\n", time);
		if (options.outfilename)
			fwrite_ints(G->marks, G->numVertices, options.outfilename);
	}


	/*------------------------------------------------------------------------- */
	/* Modularity                                                               */
	/*------------------------------------------------------------------------- */

	if (options.kernel_id == 2) {
		printf("\nFinding connected components...\n");
		fflush (stdout);
		numComp = connectedComponents(G);
		time = timer();
		printf("There are %ld components in the graph.\n", numComp);
		time = timer() - time;
		printf("Time taken is %9.6lf sec.\n", time);

		printf("\nCalculating the modularity score of the graph...\n");
		fflush (stdout);
		time = timer();
		stats_tic ("modularity");
		computeModularityValue(G, colors, NV);
		stats_toc ();
		time = timer() - time;
		printf("Time taken is %9.6lf sec.\n", time);
	}


	/*------------------------------------------------------------------------- */
	/* Conductance                                                              */
	/*------------------------------------------------------------------------- */

	if (options.kernel_id == 4) {
		printf("\nFinding connected components...\n");
		fflush (stdout);
		numComp = connectedComponents(G);
		printf("There are %ld components in the graph.\n", numComp);
		time = timer();
		calculateComponentDistributions(G, numComp, &max, &maxV);
		time = timer() - time;
		printf("Time taken is %9.6lf sec.\n", time);

MTA("mta assert nodep")
		for (i = 0; i < NV; i++ ) {
			if (colors[i] == maxV) colors[i] = 1;
			else colors[i] = 0;
		}

		printf("Calculating conductance of the largest connected component...\n");
		fflush (stdout);
		time = timer();
		stats_tic ("conductance");
		computeConductanceValue(G, colors);
		stats_toc ();
		time = timer() - time;
		printf("Time taken is %9.6lf sec.\n", time);
	}


	/*------------------------------------------------------------------------- */
	/* Clustering Coefficients                                                  */
	/*------------------------------------------------------------------------- */

	if (options.kernel_id == 6) {
		printf("\nCalculating local clustering coefficients...\n");
		fflush (stdout);
		SortGraphEdgeLists(G);
		time = timer();
		stats_tic ("clusteringcoeff");
		trans = calculateClusteringLocal(G);
		stats_toc ();
		time = timer() - time;
		printf("Time taken is %9.6lf sec.\n", time);
		if (options.outfilename)
			fwrite_doubles(trans, G->numVertices, options.outfilename);
		free(trans);
	}
	 

	/*------------------------------------------------------------------------- */
	/* Transitivity Coefficients                                                */
	/*------------------------------------------------------------------------- */

	if (options.kernel_id == 7) {
		printf("\nCalculating the global transitivity coefficient...\n");
		fflush (stdout);
		time = timer();
		stats_tic ("transitivity");
		trans = calculateTransitivityLocal(G);
		stats_toc ();
		time = timer() - time;
		printf("Time taken is %9.6lf sec.\n", time);
		if (options.outfilename)
			fwrite_doubles(trans, G->numVertices, options.outfilename);
		free(trans);
	}
	
	
	/*------------------------------------------------------------------------- */
	/* CNM style partitioning                                                   */
	/*------------------------------------------------------------------------- */

	if (options.kernel_id == 9) {
		int64_t mode = options.parameter1;
		if (mode == 1) {
			printf("\nCalculating greedy agglomerative partitioning using heaviest edges...\n");
			fflush (stdout);
			time = timer();
			agglomerative_partitioning(G, NULL, mode);
			time = timer() - time;
			printf("Time taken is %9.6lf sec.\n", time);
			if (options.outfilename)
				fwrite_ints(G->marks, G->numVertices,
					    options.outfilename);
		}
		else if (mode == 2) {
			printf("\nCalculating greedy agglomerative partitioning using CNM...\n");
			fflush (stdout);
			time = timer();
			agglomerative_partitioning(G, NULL, mode);
			time = timer() - time;
			printf("Time taken is %9.6lf sec.\n", time);
			if (options.outfilename)
				fwrite_ints(G->marks, G->numVertices,
					    options.outfilename);
		}
		else if (mode == 3) {
			printf("\nNot yet implemented...\n");
		}
		else if (mode == 4) {
			printf("\nNot yet implemented...\n");
		}
	}

	if (options.printstats)
		print_stats ();

/*
	free(G->marks);
	free(G->startVertex);
	free(G->endVertex);
	free(G->intWeight);
	free(G->edgeStart);
	free(G);   */
	return 0;
}

