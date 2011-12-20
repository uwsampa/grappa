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

static FILE* dotfile;

int main(int argc, char* argv[]) {	
	parseOptions(argc, argv);
	setupParams(SCALE, 8);
	
	double time;
	
	printf("[[ Graph Application Suite ]]\n"); 	
	
	printf("\nScalable Data Generator - genScalData() beginning execution...\n");
	fflush (stdout);
	time = timer();
	genScalData(&SDGdata, 0.55, 0.1, 0.1, 0.25);
	/* gen1DTorus(SDGdata); */
	
	time  = timer() - time;
	
	printf("generating random edgelist data\n");
	edgelist ing;
	genScalData(&ing, A, B, C, D);
	
	if (dotfile)
		print_edgelist_dot(&ing, dotfile);

	printf("Time taken for Scalable Data Generation is %9.6lf sec.\n", time);

	/* From the input edges, construct the graph 'G'.  */
	printf("\nKernel 1 - computeGraph() beginning execution...\n"); fflush(stdout);
	time = timer();
	
	graph g;
	computeGraph(&g, &ing);

	time = timer() - time;
	printf("Time taken for computeGraph (Kernel 1) is %9.6lf sec.\n", time);
	
	free_edgelist(&ing);
	
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
	dotfile = NULL;
	
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
				dotfile = fopen(optarg,"w");
				break;
		}
	}
}

