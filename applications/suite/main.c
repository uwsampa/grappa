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
#else
#include "compat/getopt.h"
#endif
#endif

#if defined(__MTA__)
#include <sys/types.h>
#include <sys/mman.h>
#endif

// includes XMT version of fread & fwrite which flip endianness
#include "../graph500/compatio.h"

#include <stdbool.h>
#include <assert.h>
#include <string.h>

#include "defs.h"

static void printHelp(const char * exe);
static void parseOptions(int argc, char ** argv);

static char* graphfile;
static graphint kcent;
static bool do_components = false,
            do_pathiso = false,
            do_triangles = false,
            do_centrality = false;

#define min(a,b) (a)<(b) ? (a) : (b)

//static void graph_in(graph * g, FILE * fin) {
//  fread(&g->numVertices, sizeof(graphint), 1, fin);
//  fread(&g->numEdges, sizeof(graphint), 1, fin);
//  
//  graphint NV = g->numVertices;
//  graphint NE = g->numEdges;
//  
//  alloc_graph(g, NV, NE);
//  
//  /* now read in contents... */
//  fread(g->startVertex, sizeof(graphint), NE, fin);
//  fread(g->endVertex, sizeof(graphint), NE, fin);
//  fread(g->edgeStart, sizeof(graphint), NV+2, fin);
//  fread(g->intWeight, sizeof(graphint), NE, fin);
//  fread(g->marks, sizeof(color_t), NV, fin);
//}
//
//static void graph_out(graph * g, FILE * fin) {
//  fwrite(&g->numVertices, sizeof(graphint), 1, fin);
//  fwrite(&g->numEdges, sizeof(graphint), 1, fin);
//  
//  graphint NV = g->numVertices;
//  graphint NE = g->numEdges;
//  
//  /* now read in contents... */
//  fwrite(g->startVertex, sizeof(graphint), NE, fin);
//  fwrite(g->endVertex, sizeof(graphint), NE, fin);
//  fwrite(g->edgeStart, sizeof(graphint), NV+2, fin);
//  fwrite(g->intWeight, sizeof(graphint), NE, fin);
//  fwrite(g->marks, sizeof(color_t), NV, fin);
//}
//
//static bool checkpoint_in(graph * dirg, graph * g) {
//  fprintf(stderr,"start reading checkpoint\n");
//  double t = timer();
//  
//  char fname[256];
//  sprintf(fname, "ckpts/suite.%d.ckpt", SCALE);
//  FILE * fin = fopen(fname, "r");
//  if (!fin) {
//    fprintf(stderr, "Unable to open file: %s, will generate graph and write checkpoint.\n", fname);
//    return false;
//  }
//  
//  graph_in(dirg, fin);
//  graph_in(g, fin);
//  
//  fclose(fin);
//  
//  t = timer() - t;
//  fprintf(stderr, "done reading in checkpoint (time = %g)\n", t);
//
//  return true;
//}

#define NBUF (1L<<12)

static void read_endVertex(int64_t * endVertex, int64_t nadj, FILE * fin, int64_t * xoff, int64_t * edgeStart, int64_t nv) {
#ifdef __MTA__
  int64_t * buf = (int64_t*)xmmap(NULL, sizeof(int64_t)*nadj, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, 0,0);
  int64_t pos = 0;
  fread(buf, sizeof(int64_t), nadj, fin);
  
  MTA("mta assert nodep")
  for (int64_t i=0; i<nv; i++) {
    int64_t d = xoff[2*i+1] - xoff[2*i];
    MTA("mta assert nodep")
    for (int64_t j=0; j<d; j++) {
      endVertex[edgeStart[i]+j] = buf[xoff[2*i]+j];
    }
  }

  munmap((caddr_t)buf, sizeof(int64_t)*nadj);
#else
  int64_t * buf = (int64_t*)xmalloc(NBUF*sizeof(int64_t));
  int64_t pos = 0;
  /*printf("endVertex: [ ");*/
  for (int64_t i=0; i<nadj; i+=NBUF) {
    int64_t n = min(nadj-i, NBUF);
    fread(buf, sizeof(int64_t), n, fin);
    for (int64_t j=0; j<n; j++) {
      if (buf[j] != -1) {
        endVertex[pos] = buf[j];
        /*printf("%ld ", endVertex[pos]);*/
        pos++;
      }
    }
  }
  /*printf("]\n");*/
  /*printf("pos = %ld\n", pos); fflush(stdout);*/
  free(buf);
#endif
}

bool checkpoint_in(graphedges * ge, graph * g) {
  int64_t buf[NBUF];

  fprintf(stderr, "starting to read ckpt...\n");
  double t = timer();

  char fname[256];
  sprintf(fname, "ckpts/graph500.%lld.%lld.xmt.w.ckpt", SCALE, 16);
  FILE * fin = fopen(fname, "r");
  if (!fin) {
    fprintf(stderr, "Unable to open file - %s.\n", fname);
    exit(1);
  }

  int64_t nedge, nv, nadj, nbfs;
  fread(&nedge, sizeof(nedge), 1, fin);
  fread(&nv,    sizeof(nv),    1, fin);
  fread(&nadj,  sizeof(nadj),  1, fin);
  fread(&nbfs,  sizeof(nbfs),  1, fin);
  fprintf(stderr, "nedge=%ld, nv=%ld, nadj=%ld, nbfs=%ld\n", nedge, nv, nadj, nbfs);

  double tt = timer();
  alloc_edgelist(ge, nedge);
  for (size_t i=0; i<nedge; i+=NBUF/2) {
    int64_t n = min(NBUF/2, nedge-i);
    fread(buf, sizeof(int64_t), 2*n, fin);
    //for (size_t j=0; j<n; j++) {
    //  ge->startVertex[i+j] = buf[2*j];
    //  ge->endVertex[i+j] = buf[2*j+1];
    //}
  }
  printf("edgelist read time: %g\n", timer()-tt);

  tt = timer();
  int64_t * xoff = xmalloc((2*nv+2)*sizeof(int64_t));
  fread(xoff, sizeof(int64_t), 2*nv+2, fin);
  printf("xoff read time: %g\n", timer()-tt);
 
  tt = timer();
  int64_t actual_nadj = 0;
  for (size_t i=0; i<nv; i++) { actual_nadj += xoff[2*i+1] - xoff[2*i]; }
  printf("actual_nadj compute time: %g\n", timer()-tt);

  alloc_graph(g, nv, actual_nadj);

  // xoff/edgeStart
  tt = timer();
  int64_t deg = 0;
  for (size_t i=0; i<nv; i++) {
    g->edgeStart[i] = deg;
    deg += xoff[2*i+1] - xoff[2*i];
  }
  g->edgeStart[nv] = deg;
  printf("edgeStart compute time: %g\n", timer()-tt);

  // xadj/endVertex
  // eat first 2 because we actually stored 'xadjstore' which has an extra 2 elements
  tt = timer();
  fread(buf, sizeof(int64_t), 2, fin); nadj -= 2;
  read_endVertex(g->endVertex, nadj, fin, xoff, g->edgeStart, nv);
  printf("endVertex read time: %g\n", timer()-tt);
  //int64_t pos = 0; 
  //for (int64_t v=0; v<nv; v++) {
  //  int64_t d = xoff[2*v+1]-xoff[2*v];
  //  fread(g->endVertex+g->edgeStart[v], sizeof(int64_t), d, fin); pos += d;
  //  // eat up to next one
  //  d = (v+1==nv) ? nadj-pos : xoff[2*(v+1)]-pos;
  //  for (int64_t j=0; j < d; j+=NBUF) {
  //    int64_t n = min(NBUF, d-j);
  //    fread(buf, sizeof(int64_t), n, fin); pos += n;
  //  }
  //}
  free(xoff);

  // startVertex
  tt = timer();
  for (int64_t v=0; v<nv; v++) {
    for (int64_t j=g->edgeStart[v]; j<g->edgeStart[v+1]; j++) {
      g->startVertex[j] = v;
    }
  }
  printf("startVertex time: %g\n", timer()-tt);

  // bfs roots (don't need 'em)
  CHECK(NBUF > nbfs) { fprintf(stderr, "too many bfs\n"); }
  fread(buf, sizeof(int64_t), nbfs, fin);

  // int weights
  tt = timer();
  int64_t nw;
  fread(&nw, sizeof(int64_t), 1, fin);
  CHECK(nw == actual_nadj) { fprintf(stderr, "nw = %ld, actual_nadj = %ld\n", nw, actual_nadj); }
  fread(g->intWeight, sizeof(int64_t), nw, fin);
  printf("intWeight read time: %g\n", timer()-tt);
  /*for (int64_t i=0; i<nw; i++) { fprintf(stdout, "%ld ", g->intWeight[i]); } fprintf(stdout, "\n"); fflush(stdout);*/

  fprintf(stderr, "checkpoint_read_time: %g\n", timer()-t);
  return true;
}

//static void checkpoint_out(graph * dirg, graph * undirg) {
//  fprintf(stderr, "starting checkpoint output\n");
//  double t = timer();
//  
//  char fname[256];
//  sprintf(fname, "ckpts/suite.%d.ckpt", SCALE);
//  FILE * fout = fopen(fname, "w");
//  if (!fout) {
//    fprintf(stderr, "Unable to open file for writing: %s.\n", fname);
//    exit(1);
//  }
//  
//  graph_out(dirg, fout);
//  graph_out(undirg, fout);
//  
//  fclose(fout);
//  
//  t = timer() - t;
//  fprintf(stderr, "done writing checkpoint (time = %g)\n", t);
//}

int main(int argc, char* argv[]) {	
  parseOptions(argc, argv);
  setupParams(SCALE, 8);
  
  graphedges _ge;
  graph _dirg;
  graph _g;
  
  double t;
  
  printf("[[ Graph Application Suite ]]\n"); fflush(stdout);
  
  graph * dirg = &_dirg;
  graph * g = &_g;
  graphedges * ge = &_ge;
  
  bool generate_checkpoint = false;

  if (checkpointing) {
    generate_checkpoint = !checkpoint_in(ge, g);
  }
  
  if (!checkpointing || generate_checkpoint) {
    printf("\nScalable Data Generator - genScalData() randomly generating edgelist...\n"); fflush(stdout);
    t = timer();
    
    genScalData(ge, A, B, C, D);
    
    t = timer() - t;
    printf("edge_generation_time: %g\n", t);
    if (graphfile) print_edgelist_dot(ge, graphfile);
    
    //###############################################
    // Kernel: Compute Graph
    
    /* From the input edges, construct the graph 'G'.  */
    printf("Kernel - Compute Graph beginning execution...\n"); fflush(stdout);
    
    t = timer();
    
    // directed graph
    computeGraph(ge, dirg);
    free_edgelist(ge);
    
    // undirected graph
    makeUndirected(dirg, g);
    
    t = timer() - t;
    printf("compute_graph_time: %g\n", t);
    if (graphfile) print_graph_dot(g, graphfile);
    
    /*if (generate_checkpoint) checkpoint_out(dirg, g);*/
  }
  
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
    markColors(g, 0, 10);
    
    // path to find (sequence of specifically colored vertices)
    color_t pattern[] = {2, 5, 9, END};
    
    color_t *c = pattern;
    printf("Kernel - Path Isomorphism beginning execution...\nfinding path: %"DFMT"", *c);
    c++; while (*c != END) { printf(" -> %"DFMT"", *c); c++; } printf("\n"); fflush(stdout);
    t = timer();
    
    graphint num_matches = pathIsomorphismPar(g, pattern);
    
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
    printf("Kernel - Betweenness Centrality (kcent = %ld) beginning execution...\n", kcent); fflush(stdout);
    t = timer();
    
    double *bc = xmalloc(numVertices*sizeof(double));
    int64_t total_nedge;
    double avgbc = centrality(g, bc, kcent, &total_nedge);
    
    t = timer() - t;
    printf("avg_centrality: %12.10g\n", avgbc);
    printf("centrality_time: %g\n", t); fflush(stdout);
    printf("centrality_teps: %g\n", total_nedge / t);
  }
  
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
  enum GraphKernel { KERNEL_COMPONENTS, KERNEL_PATHISO, KERNEL_TRIANGLES, KERNEL_CENTRALITY };
  struct option long_opts[] = {
    {"help", no_argument, 0, 'h'},
    {"scale", required_argument, 0, 's'},
    {"dot", required_argument, 0, 'd'},
    {"ckpt", no_argument, 0, 'p'},
    {"kcent", required_argument, 0, 'c'},
    {"components", no_argument, 0, KERNEL_COMPONENTS},
    {"pathiso",    no_argument, 0, KERNEL_PATHISO},
    {"triangles",  no_argument, 0, KERNEL_TRIANGLES},
    {"centrality", no_argument, 0, KERNEL_CENTRALITY}
  };
  
  SCALE = 8; //default value
  graphfile = NULL;
  kcent = 1L << SCALE;
  checkpointing = false;

  int c = 0;
  while (c != -1) {
    int option_index = 0;
    
    c = getopt_long(argc, argv, "hs:d:pc:", long_opts, &option_index);
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
      case KERNEL_COMPONENTS:
        do_components = true;
        break;
      case KERNEL_PATHISO:
        do_pathiso = true;
        break;
      case KERNEL_TRIANGLES:
        do_triangles = true;
        break;
      case KERNEL_CENTRALITY:
        do_centrality = true;
        break;
    }
  }

  // if no flags set, default to doing all
  if (!(do_components || do_pathiso || do_triangles || do_centrality)) {
    do_components = do_pathiso = do_triangles = do_centrality = true;
  }
}

