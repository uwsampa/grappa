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


int main(int argc, char* argv[]) {
	parseOptions(argc, argv);
	setupParams(SCALE, 8);
	
	printf("[[ Graph Application Suite ]]\n"); 	
	
	printf("generating random edgelist data\n");
	edgelist ing;
	genScalData(&ing, A, B, C, D);
	
	
	free_edgelist(&ing);
	
	return 0;
}

static void printHelp(const char * exe) {
	printf("Usage: %s [options]\nOptions:\n", exe);
	printf("  --help,h    Prints this help message displaying command-line options\n");
	printf("  --scale,s  Number of time steps to simulate\n");
	exit(0);
}

static void parseOptions(int argc, char ** argv) {
	struct option long_opts[] = {
		{"help", no_argument, 0, 'h'},
		{"scale", no_argument, (int*)&SCALE, true},
		{"toggle-on", no_argument, 0, 't'},
	};
	
	int c = 0;
	while (c != -1) {
		int option_index = 0;
		c = getopt_long(argc, argv, "ht", long_opts, &option_index);
		switch (c) {
			case 'h':
				printHelp(argv[0]);
				exit(0);
			case 't':
				//toggle_on
				break;
		}
	}
}

