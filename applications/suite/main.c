//
//  main.c
//  AppSuite
//
//  Created by Brandon Holt on 12/13/11.
//  Copyright 2011 University of Washington. All rights reserved.
//
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>

#include "defs.h"

static void printHelp(const char * exe);
static void parseOptions(int argc, char ** argv);

static char* graphfile;

int main(int argc, char* argv[]) {	
	parseOptions(argc, argv);
	setupParams(SCALE, 8);
	
	double time;
	
	printf("[[ Graph Application Suite ]]\n"); 	
	
	printf("\nScalable Data Generator - genScalData() randomly generating edgelist...\n");
	fflush (stdout);
	time = timer();
	
	graphedges* ge = xmalloc(sizeof(graphedges));
	genScalData(ge, A, B, C, D);
	
	time = timer() - time;
	printf("Time taken for Scalable Data Generation is %9.6lf sec.\n", time);
	if (graphfile) print_edgelist_dot(ge, graphfile);

	/* From the input edges, construct the graph 'G'.  */
	printf("\nKernel 1 - computeGraph() beginning execution...\n"); fflush(stdout);
	time = timer();
	
	// directed graph
	graph* dirg = computeGraph(ge);
	free_edgelist(ge);
	
	// undirected graph
	graph* g = makeUndirected(dirg);
	
	time = timer() - time;
	printf("Time taken for computeGraph (Kernel 1) is %9.6lf sec.\n", time);	
	if (graphfile) print_graph_dot(g, graphfile);
	
	// Kernel 2: Connected Components
	
	free_graph(dirg);
	free_graph(g);
	
	return 0;
}

static void printHelp(const char * exe) {
	printf("Usage: %s [options]\nOptions:\n", exe);
	printf("  --help,h   Prints this help message displaying command-line options\n");
	printf("  --scale,s  Scale of the graph: 2^SCALE vertices.\n");
	printf("  --dot,d    Filename for output of graph in dot form.\n");
	exit(0);
}

static void parseOptions(int argc, char ** argv) {
	struct option long_opts[] = {
		{"help", no_argument, 0, 'h'},
		{"scale", required_argument, 0, 's'},
		{"dot", required_argument, 0, 'd'}
	};
	
	SCALE = 1; //default value
	graphfile = NULL;
	
	int c = 0;
	while (c != -1) {
		int option_index = 0;
		c = getopt_long(argc, argv, "hsd", long_opts, &option_index);
		switch (c) {
			case 'h':
				printHelp(argv[0]);
				exit(0);
			case 's':
				SCALE = atoi(optarg);
				break;
			case 'd':
				graphfile = optarg;
				break;
		}
	}
}

