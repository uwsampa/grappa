//
//  main.c
//  AppSuite
//
//  Created by Brandon Holt on 12/13/11.
//  Copyright 2011 University of Washington. All rights reserved.
//
#if HAVE_UNISTD_H
#include <unistd.h>
#else
#if !defined(__MTA__)
#include <getopt.h>
#endif
#endif

#include <stdio.h>

#include "defs.hpp"
#include <SoftXMT.hpp>
#include <GlobalAllocator.hpp>

static void printHelp(const char * exe);
static void parseOptions(int argc, char ** argv);

/////////////////
// Globals
/////////////////
static char* graphfile;
static graphint kcent;
static bool do_components = false,
            do_pathiso = false,
            do_triangles = false,
            do_centrality = false;

double A, B, C, D;
int SCALE;
graphint numVertices;
graphint numEdges;
graphint maxWeight;
int K4approx;
graphint subGraphPathLength;

bool checkpointing;

#define MAX_ACQUIRE_SIZE (1L<<20)

template< typename T >
static void read_array(GlobalAddress<T> base_addr, size_t nelem, FILE* fin) {
  size_t bufsize = MAX_ACQUIRE_SIZE / sizeof(T);
  T* buf = new T[bufsize];
  for (size_t i=0; i<nelem; i+=bufsize) {
    size_t n = (nelem-i < bufsize) ? nelem-i : bufsize;
    typename Incoherent<T>::WO c(base_addr+i, n, buf);
    c.block_until_acquired();
    size_t nread = fread(buf, sizeof(T), n, fin);
    CHECK(nread == n) << nread << " : " << n;
    c.block_until_released();
  }
  delete [] buf;
}

static void graph_in(graph * g, FILE * fin) {
  fread(&g->numVertices, sizeof(graphint), 1, fin);
  fread(&g->numEdges, sizeof(graphint), 1, fin);
  
  graphint NV = g->numVertices;
  graphint NE = g->numEdges;
  
  alloc_graph(g, NV, NE);
  
  /* now read in contents... */
  read_array(g->startVertex, NE, fin);
  read_array(g->endVertex, NE, fin);
  read_array(g->edgeStart, NV+2, fin);
  read_array(g->intWeight, NE, fin);
  read_array(g->marks, NV, fin);
}

static void checkpoint_in(graph * dirg, graph * g) {
  printf("start reading checkpoint\n"); fflush(stdout);
  double t = timer();
  
  char fname[256];
  sprintf(fname, "../ckpts/suite.%d.ckpt", SCALE);
  FILE * fin = fopen(fname, "r");
  if (!fin) {
    fprintf(stderr, "Unable to open file: %s, will generate graph and write checkpoint.\n", fname);
    checkpointing = false;
    return;
  }
  
  VLOG(1) << "about to load dirg";
  graph_in(dirg, fin);
  VLOG(1) << "about to load g";
  graph_in(g, fin);
  
  fclose(fin);
  
  t = timer() - t;
  printf("done reading in checkpoint (time = %g)\n", t); fflush(stdout);
}

static void user_main(void* ignore) {
  double t;
  
  graphedges _ge;
  graph _dirg;
  graph _g;
  
  printf("[[ Graph Application Suite ]]\n"); fflush(stdout);	
  
  graph * dirg = &_dirg;
  graph * g = &_g;
  
  if (checkpointing) {
    checkpoint_in(dirg, g);
  }
  
  if (!checkpointing) {
    fprintf(stderr, "graph generation not implemented for Grappa yet...\n");
    exit(0);
    
    printf("\nScalable Data Generator - genScalData() randomly generating edgelist...\n"); fflush(stdout);
    t = timer();
    
    graphedges * ge = &_ge;
    genScalData(ge, A, B, C, D);
    
    t = timer() - t;
    printf("edge_generation_time: %g sec.\n", t);
    //	if (graphfile) print_edgelist_dot(ge, graphfile);
    
    //###############################################
    // Kernel: Compute Graph
    
    /* From the input edges, construct the graph 'G'.  */
    printf("Kernel - Compute Graph beginning execution...\n"); fflush(stdout);
    //	MTA("mta trace \"begin computeGraph\"")
    
    t = timer();
    
    // directed graph
    computeGraph(ge, dirg);
    
    //	free_edgelist(&ge);
    
    // undirected graph
    makeUndirected(dirg, g);
    
    t = timer() - t;
    printf("compute_graph_time: %g\n", t);
  }
  
  SoftXMT_reset_stats();
  
  //###############################################
  // Kernel: Connected Components
  if (do_components) {
    printf("Kernel - Connected Components beginning execution...\n"); fflush(stdout);
    t = timer();
    
    graphint connected = connectedComponents(g);
    
    t = timer() - t;
    printf("ncomponents: %"DFMT"\n", connected);
    printf("components_time: %g\n", t); fflush(stdout);
  }  
  
  //###############################################
  // Kernel: Path Isomorphism
  if (do_pathiso) {
    // assign random colors to vertices in the range: [0,10)
    //	MTA("mta trace \"begin markColors\"")
    markColors(dirg, 0, 10);
    
    // path to find (sequence of specifically colored vertices)
    color_t pattern[] = {2, 5, 9};
    size_t npattern = 3;
    
    color_t *c = pattern;
    printf("Kernel - Path Isomorphism beginning execution...\nfinding path: %"DFMT"", *c);
    for (color_t * c = pattern+1; c < pattern+npattern; c++) { printf(" -> %"DFMT"", *c); } printf("\n"); fflush(stdout);
    t = timer();
    
    graphint num_matches = pathIsomorphism(dirg, pattern, npattern);
    
    t = timer() - t;
    printf("path_iso_matches: %"DFMT"\n", num_matches);
    printf("path_isomorphism_time: %g\n", t); fflush(stdout);
  }
    
  //###############################################
  // Kernel: Triangles
  if (do_triangles) {
    printf("Kernel - Triangles beginning execution...\n"); fflush(stdout);
    t = timer();
    
    graphint num_triangles = triangles(g);
    
    t = timer() - t;
    printf("ntriangles: %"DFMT"\n", num_triangles);
    printf("triangles_time: %g\n", t); fflush(stdout);
  }
  
  //###############################################
  // Kernel: Betweenness Centrality
  if (do_centrality) {
    printf("Kernel - Betweenness Centrality beginning execution...\n"); fflush(stdout);
    
    GlobalAddress<double> bc = SoftXMT_typed_malloc<double>(numVertices);
    
    double avgbc;
    int64_t total_nedge;
    t = centrality(g, bc, kcent, &avgbc, &total_nedge);
    
    printf("avg_centrality: %g\n", avgbc);
    printf("centrality_time: %g\n", t); fflush(stdout);
    printf("centrality_teps: %g\n", (double)total_nedge / t);
  }
  
  //###################
  // Kernels complete!
  
  SoftXMT_merge_and_dump_stats();
  //SoftXMT_dump_stats_all_nodes();

  VLOG(1) << "freeing graphs";
  free_graph(dirg);
  free_graph(g);
  VLOG(1) << "done freeing";
}

int main(int argc, char* argv[]) {
  SoftXMT_init(&argc, &argv, 1L<<30);
  SoftXMT_activate();
  
  parseOptions(argc, argv);
  setupParams(SCALE, 8);
  
  SoftXMT_run_user_main(&user_main, (void*)NULL);
  
  VLOG(1) << "finishing...";
  SoftXMT_finish(0);
  return 0;
}

void setupParams(int scale, int edgefactor) {
  /* SCALE */                             /* Binary scaling heuristic  */
  A                  = 0.55;              /* RMAT parameters           */
  B                  = 0.1;
  C                  = 0.1;
  D                  = 0.25;
  numVertices        =  (1 << scale);     /* Total num of vertices     */
  numEdges           =  edgefactor * numVertices;  /* Total num of edges        */
  maxWeight          =  (1 << scale);     /* Max integer weight        */
  K4approx           =  8;                /* Scale factor for K4       */
  subGraphPathLength =  3;                /* Max path length for K3    */
}

static void printHelp(const char * exe) {
  printf("Usage: %s [options]\nOptions:\n", exe);
  printf("  --help,h   Prints this help message displaying command-line options\n");
  printf("  --scale,s  Scale of the graph: 2^SCALE vertices.\n");
  printf("  --dot,d    Filename for output of graph in dot form.\n");
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
    {"kcent", required_argument, 0, 'c'},
    {"components", no_argument, (int*)&do_components, true},
    {"pathiso", no_argument, (int*)&do_pathiso, true},
    {"triangles", no_argument, (int*)&do_triangles, true},
    {"centrality", no_argument, (int*)&do_centrality, true}
  };
  
  SCALE = 8; //default value
  graphfile = NULL;
  checkpointing = false;
  kcent = 1L << SCALE;
  
  int c = 0;
  while (c != -1) {
    int option_index = 0;
    
    c = getopt_long(argc, argv, "hsdpck", long_opts, &option_index);
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
  
  // if no flags set, default to doing all
  if (!(do_components || do_pathiso || do_triangles || do_centrality)) {
    do_components = do_pathiso = do_triangles = do_centrality = true;
  }
}

