//
//  main.c
//  AppSuite
//
//  Created by Brandon Holt on 12/13/11.
//  Copyright 2011 University of Washington. All rights reserved.
//
#include <stdio.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#else
#if !defined(__MTA__)
#include <getopt.h>
#endif
#endif

#include <stdbool.h>
#include <assert.h>
#include <string.h>

#include "defs.h"

static void printHelp(const char * exe);
static void parseOptions(int argc, char ** argv);

static char* graphfile;
static graphint kcent;

static void graph_in(graph * g, FILE * fin) {
  fread(&g->numVertices, sizeof(graphint), 1, fin);
  fread(&g->numEdges, sizeof(graphint), 1, fin);
  
  graphint NV = g->numVertices;
  graphint NE = g->numEdges;
  
  alloc_graph(g, NV, NE);
  
  /* now read in contents... */
  fread(g->startVertex, sizeof(graphint), NE, fin);
  fread(g->endVertex, sizeof(graphint), NE, fin);
  fread(g->edgeStart, sizeof(graphint), NV+2, fin);
  fread(g->intWeight, sizeof(graphint), NE, fin);
  fread(g->marks, sizeof(color_t), NV, fin);
}

static void graph_out(graph * g, FILE * fin) {
  fwrite(&g->numVertices, sizeof(graphint), 1, fin);
  fwrite(&g->numEdges, sizeof(graphint), 1, fin);
  
  graphint NV = g->numVertices;
  graphint NE = g->numEdges;
  
  /* now read in contents... */
  fwrite(g->startVertex, sizeof(graphint), NE, fin);
  fwrite(g->endVertex, sizeof(graphint), NE, fin);
  fwrite(g->edgeStart, sizeof(graphint), NV+2, fin);
  fwrite(g->intWeight, sizeof(graphint), NE, fin);
  fwrite(g->marks, sizeof(color_t), NV, fin);
}

static void checkpoint_in(graph * dirg, graph * g) {
  fprintf(stderr,"start reading checkpoint\n");
  double t = timer();
  
  char fname[256];
  sprintf(fname, "ckpts/suite.%d.ckpt", SCALE);
  FILE * fin = fopen(fname, "r");
  if (!fin) {
    fprintf(stderr, "Unable to open file: %s, will generate graph and write checkpoint.\n", fname);
    checkpointing = false;
    return;
  }
  
  graph_in(dirg, fin);
  graph_in(g, fin);
  
  fclose(fin);
  
  t = timer() - t;
  fprintf(stderr, "done reading in checkpoint (time = %g)\n", t);
}

static void checkpoint_out(graph * dirg, graph * undirg) {
  fprintf(stderr, "starting checkpoint output\n");
  double t = timer();
  
  char fname[256];
  sprintf(fname, "ckpts/suite.%d.ckpt", SCALE);
  FILE * fout = fopen(fname, "w");
  if (!fout) {
    fprintf(stderr, "Unable to open file for writing: %s.\n", fname);
    exit(1);
  }
  
  graph_out(dirg, fout);
  graph_out(undirg, fout);
  
  fclose(fout);
  
  t = timer() - t;
  fprintf(stderr, "done writing checkpoint (time = %g)\n", t);
}

int main(int argc, char* argv[]) {	
  parseOptions(argc, argv);
  setupParams(SCALE, 8);
  
  graphedges _ge;
  graph _dirg;
  graph _g;
  
  double time;
  
  printf("[[ Graph Application Suite ]]\n"); fflush(stdout);
  
  graph * dirg = &_dirg;
  graph * g = &_g;
  
  if (checkpointing) {
    checkpoint_in(dirg, g);
  }
  
  if (!checkpointing) {
    graphedges * ge = &_ge;
    
    printf("\nScalable Data Generator - genScalData() randomly generating edgelist...\n"); fflush(stdout);
    //	MTA("mta trace \"begin genScalData\"")
    time = timer();
    
    
    genScalData(ge, A, B, C, D);
    
    time = timer() - time;
    printf("Time taken for Scalable Data Generation is %9.6lf sec.\n", time);
    if (graphfile) print_edgelist_dot(ge, graphfile);
    
    //###############################################
    // Kernel: Compute Graph
    
    /* From the input edges, construct the graph 'G'.  */
    printf("\nKernel - Compute Graph beginning execution...\n"); fflush(stdout);
    //	MTA("mta trace \"begin computeGraph\"")
    
    time = timer();
    
    // directed graph
    computeGraph(ge, dirg);
    free_edgelist(ge);
    
    // undirected graph
    makeUndirected(dirg, g);
    
    time = timer() - time;
    printf("Time taken for computeGraph is %9.6lf sec.\n", time);
    if (graphfile) print_graph_dot(g, graphfile);
    
    checkpoint_out(dirg, g);
  }
  
  //###############################################
  // Kernel: Connected Components
  printf("\nKernel - Connected Components beginning execution...\n"); fflush(stdout);
  MTA("mta trace \"begin connectedComponents\"")
  time = timer();
  
  graphint connected = connectedComponents(g);
  
  time = timer() - time;
  printf("Number of connected components: %"DFMT"\n", connected);
  printf("Time taken for connectedComponents is %9.6lf sec.\n", time);
  
  
  //###############################################
  // Kernel: Path Isomorphism
  
  // assign random colors to vertices in the range: [0,10)
  //	MTA("mta trace \"begin markColors\"")
  markColors(dirg, 0, 10);
  
  // path to find (sequence of specifically colored vertices)
  color_t pattern[] = {2, 5, 9, END};
  
  color_t *c = pattern;
  printf("\nKernel - Path Isomorphism beginning execution...\nfinding path: %"DFMT"", *c);
  c++; while (*c != END) { printf(" -> %"DFMT"", *c); c++; } printf("\n"); fflush(stdout);
  //	MTA("mta trace \"begin pathIsomorphism\"")
  time = timer();
  
  graphint num_matches = pathIsomorphismPar(dirg, pattern);
  
  time = timer() - time;
  
  printf("Number of matches: %"DFMT"\n", num_matches);
  printf("Time taken for pathIsomorphismPar is %9.6lf sec.\n", time);
  
  //###############################################
  // Kernel: Triangles
  printf("\nKernel - Triangles beginning execution...\n"); fflush(stdout);
  //	MTA("mta trace \"begin triangles\"")
  time = timer();
  
  graphint num_triangles = triangles(g);
  
  time = timer() - time;
  printf("Number of triangles: %"DFMT"\n", num_triangles);
  printf("Time taken for triangles is %9.6lf sec.\n", time);
  
  
  //###############################################
  // Kernel: Betweenness Centrality
  printf("\nKernel - Betweenness Centrality beginning execution...\n"); fflush(stdout);
  //	MTA("mta trace \"begin centrality\"")
  time = timer();
  
  double *bc = xmalloc(numVertices*sizeof(double));
  double avgbc = centrality(g, bc, 10);
  
  time = timer() - time;
  printf("Betweenness Centrality: (avg) = %lf\n", avgbc);
  printf("Time taken for betweenness centrality is %9.6lf sec.\n", time);
  
  //###################
  // Kernels complete!
  free_graph(dirg);
  free_graph(g);
  
  return 0;
}

static void printHelp(const char * exe) {
  printf("Usage: %s [options]\nOptions:\n", exe);
  printf("  --help,h   Prints this help message displaying command-line options\n");
  printf("  --scale,s  Scale of the graph: 2^SCALE vertices.\n");
  printf("  --dot,d         Filename for output of graph in dot form.\n");
  printf("  --ckpt,p  Save/read computed graphs to/from file.\n");
  printf("  --kcent,c  Number of vertices to start centrality calculation from.\n");
  exit(0);
}

static void parseOptions(int argc, char ** argv) {
  struct option long_opts[] = {
    {"help", no_argument, 0, 'h'},
    {"scale", required_argument, 0, 's'},
    {"dot", required_argument, 0, 'd'},
    {"ckpt", no_argument, 0, 'p'},
    {"kcent", required_argument, 0, 'c'}
  };
  
  SCALE = 8; //default value
  graphfile = NULL;
  kcent = 1L << SCALE;
  
  int c = 0;
  while (c != -1) {
    int option_index = 0;
    
    c = getopt_long(argc, argv, "hsdpc", long_opts, &option_index);
    switch (c) {
      case 'h':
        printHelp(argv[0]);
        exit(0);
      case 's':
        SCALE = atoi(optarg);
        break;
      case 'c':
        kcent = atoi(optarg);
        break;
      case 'd':
        graphfile = optarg;
        break;
      case 'p':
        checkpointing = true;
        break;
    }
  }
}

