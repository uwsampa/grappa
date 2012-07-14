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
#include <math.h>

#include "defs.hpp"
#include <SoftXMT.hpp>
#include <GlobalAllocator.hpp>
#include <Cache.hpp>
#include <ForkJoin.hpp>
#include <GlobalTaskJoiner.hpp>
#include <Collective.hpp>

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
#define NBUF (1L<<20)
#define NBUF_STACK (1L<<12)

static GlobalAddress<int64_t> xoff;
static graph g;
static int64_t actual_nadj;

template< typename T >
static void read_array(GlobalAddress<T> base_addr, size_t nelem, FILE* fin, T * buf = NULL, size_t bufsize = 0) {
  bool should_free = false;
  if (buf == NULL) {
    bufsize = MAX_ACQUIRE_SIZE / sizeof(T);
    buf = new T[bufsize];
    should_free = true;
  }
  for (size_t i=0; i<nelem; i+=bufsize) {
    size_t n = (nelem-i < bufsize) ? nelem-i : bufsize;
    typename Incoherent<T>::WO c(base_addr+i, n, buf);
    c.block_until_acquired();
    size_t nread = fread(buf, sizeof(T), n, fin);
    CHECK(nread == n) << nread << " : " << n;
  }
  if (should_free) delete [] buf;
}

static void read_endVertex(GlobalAddress<int64_t> endVertex, int64_t nadj, FILE * fin, int64_t * buf, size_t bufsize) {
  int64_t pos = 0;
  for (int64_t i=0; i<nadj; i+=bufsize) {
    int64_t n = min(nadj-i, bufsize);
    fread(buf, sizeof(int64_t), n, fin);
    int64_t p = 0;
    for (int64_t j=0; j<n; j++) {
      if (buf[j] != -1) {
        buf[p] = buf[j];
        p++;
      }
    }
    Incoherent<int64_t>::WO cout(endVertex+pos, p, buf);
    pos += p;
  }
}
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
//  read_array(g->startVertex, NE, fin);
//  read_array(g->endVertex, NE, fin);
//  read_array(g->edgeStart, NV+2, fin);
//  read_array(g->intWeight, NE, fin);
//  read_array(g->marks, NV, fin);
//}
//
//static void checkpoint_in(graph * dirg, graph * g) {
//  printf("start reading checkpoint\n"); fflush(stdout);
//  double t = timer();
//  
//  char fname[256];
//  sprintf(fname, "../ckpts/suite.%d.ckpt", SCALE);
//  FILE * fin = fopen(fname, "r");
//  if (!fin) {
//    fprintf(stderr, "Unable to open file (%s), will generate graph and write checkpoint.\n", fname);
//    checkpointing = false;
//    return;
//  }
//  
//  VLOG(1) << "about to load dirg";
//  graph_in(dirg, fin);
//  VLOG(1) << "about to load g";
//  graph_in(g, fin);
//  
//  fclose(fin);
//  
//  t = timer() - t;
//  printf("checkpoint_read_time: %g\n", t); fflush(stdout);
//}

void calc_actual_nadj(int64_t sv, int64_t n) {
  int64_t buf[2*n];
  Incoherent<int64_t>::RO c(xoff+2*sv, 2*n, buf);
  for (int64_t i=0; i<n; i++) {
    actual_nadj += c[2*i+1] - c[2*i];
  }
}
LOOP_FUNCTOR(func_actual_nadj, nid, ((GlobalAddress<int64_t>,_xoff)) ((int64_t,nv)) ) {
  xoff = _xoff;
  actual_nadj = 0;

  global_async_parallel_for(calc_actual_nadj, 0, nv);
  
  actual_nadj = SoftXMT_allreduce<int64_t,coll_add<int64_t>,0>(actual_nadj);
}

void set_startVertex(int64_t s, int64_t n) {
  int64_t sbuf[n];
  Incoherent<int64_t>::RO cstarts(g.edgeStart, n+1, sbuf);
  for (int64_t i=0; i<n; i++) {
    int64_t v = s+i;
    int64_t d = cstarts[i+1]-cstarts[i];
    SoftXMT_memset(g.startVertex+cstarts[i], v, d);
  }
}
LOOP_FUNCTION(func_startVertex, nid) {
  global_async_parallel_for(set_startVertex, 0, g.numVertices);
}

// TODO: need parallel prefix_sum
//void calc_edgeStart(int64_t sv, int64_t n) {
//  int64_t _estarts[n], _xoff[2*n];
//  Incoherent<int64_t>::WO ces(g.edgeStart+sv, n, _estarts);
//  Incoherent<int64_t>::RO cxoff(xoff+sv, 2*n, _xoff);
//  
//  for (int64_t i=0; i<n; i++) {
//    ces[i] = cxoff[2*i+1] - cxoff[2*i];
//  }
//}
//LOOP_FUNCTOR(func_edgeStart, nid, ((graph,_g))) {
//  g = _g;
//
//  global_async_parallel_for(calc_edgeStart, 0, g.numVertices);
//}

bool checkpoint_in(graphedges * ge, graph * g) {
  int64_t buf[NBUF_STACK];
  int64_t * rbuf = (int64_t*)malloc(2*(NBUF+1)*sizeof(int64_t));
  int64_t * wbuf = (int64_t*)malloc(NBUF*sizeof(int64_t));
  VLOG(1) << "wbuf = " << wbuf << ", wbuf[0] = " << wbuf[0];

  fprintf(stderr, "starting to read ckpt...\n");
  double t = timer();
  
  char fname[256];
  sprintf(fname, "../ckpts/graph500.%lld.%lld.xmt.w.ckpt", SCALE, 16);
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

  fprintf(stderr, "warning: skipping edgelist\n");
  fseek(fin, nedge * 2*sizeof(int64_t), SEEK_CUR);

  GlobalAddress<int64_t> xoff = SoftXMT_typed_malloc<int64_t>(2*nv+2);

  double tt = timer();
  read_array(xoff, 2*nv, fin, wbuf, NBUF);
  tt = timer() - tt; VLOG(1) << "xoff time: " << tt;
  VLOG(1) << "actual_nadj (read_array): " << actual_nadj;

  // burn extra two xoff's
  fread(wbuf, sizeof(int64_t), 2, fin);

  tt = timer();
  { func_actual_nadj f(xoff, nv); fork_join_custom(&f); }
  tt = timer() - tt; VLOG(1) << "actual_nadj: " << tt;

  VLOG(1) << "nv: " << nv << ", actual_nadj: " << actual_nadj;
  alloc_graph(g, nv, actual_nadj);

  tt = timer();
  // xoff/edgeStart
  int64_t deg = 0;
  for (int64_t i=0; i<nv; i+=NBUF) {
    int64_t n = min(nv-i, NBUF);
    Incoherent<int64_t>::RO cxoff(xoff+2*i, 2*n, rbuf);
    Incoherent<int64_t>::WO cstarts(g->edgeStart+i, n, wbuf);
    for (int64_t j=0; j<n; j++) {
      cstarts[j] = deg;
      deg += cxoff[2*j+1]-cxoff[2*j];
    }
  }
  SoftXMT_delegate_write_word(g->edgeStart+nv, deg);
  tt = timer() - tt; VLOG(1) << "edgeStart time: " << tt;

  // xadj/endVertex
  // eat first 2 because we actually stored 'xadjstore' which has an extra 2 elements
  fread(buf, sizeof(int64_t), 2, fin);

  tt = timer();
  int64_t pos = 0; nadj -= 2;
  read_endVertex(g->endVertex, nadj, fin, wbuf, NBUF);

  //for (int64_t v=0; v<nv; v+=NBUF) {
    //int64_t nstarts = min(nv-v, NBUF);
    //Incoherent<int64_t>::RO cxoff(xoff+2*v, 2*nstarts+((v+1==nv) ? 0 : 1),rbuf);
    
    //for (int64_t i=0; i<nstarts; i++) {
      //if ((v+i) % 1024 == 0) VLOG(1) << "endVertex progress (v " << v+i << "/" << nv << ")";
      //int64_t d = cxoff[2*i+1]-cxoff[2*i];
      //read_array(g->endVertex+cxoff[i], d, fin, wbuf, NBUF);
      //pos += d;
      //// eat up to next one
      //d = (v+i+1==nv) ? nadj-pos : cxoff[2*(v+i+1)]-pos;
      //for (int64_t j=0; j < d; j+=NBUF) {
        //int64_t n = min(NBUF, d-j);
        //fread(wbuf, sizeof(int64_t), n, fin); pos += n;
      //}
    //}
  //}
  SoftXMT_free(xoff);
  tt = timer() - tt; VLOG(1) << "endVertex time: " << tt;

  tt = timer();
  { func_startVertex f; fork_join_custom(&f); }
  tt = timer() - tt; VLOG(1) << "startVertex time: " << tt;

  // bfs roots (don't need 'em)
  CHECK(NBUF > nbfs) << "too many bfs";
  fread(buf, sizeof(int64_t), nbfs, fin);

  tt = timer();
  // int weights
  int64_t nw;
  fread(&nw, sizeof(int64_t), 1, fin);
  CHECK(nw == actual_nadj) << "nw = " << nw << ", actual_nadj = " << actual_nadj;
  fprintf(stderr, "warning: skipping intWeight\n");
  //read_array(g->intWeight, nw, fin);
  //tt = timer() - tt; VLOG(1) << "intWeight time: " << tt;

  fprintf(stderr, "checkpoint_read_time: %g\n", timer()-t);
  return true;
}

static void user_main(void* ignore) {
  double t;
  
  graphedges _ge;
  graph _dirg;
  graph _g;
  
  printf("[[ Graph Application Suite ]]\n"); fflush(stdout);	
  
  graphedges * ge = &_ge;
  graph * dirg = &_dirg;
  graph * g = &_g;
  
  if (checkpointing) {
    checkpoint_in(ge, g);
  }
  
  if (!checkpointing) {
    fprintf(stderr, "graph generation not implemented for Grappa yet...\n");
    exit(0);
    
    printf("\nScalable Data Generator - genScalData() randomly generating edgelist...\n"); fflush(stdout);
    t = timer();
    
    genScalData(ge, A, B, C, D);
    
    t = timer() - t;
    printf("edge_generation_time: %g\n", t);
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
    
    double ref_bc = -1;
    switch (SCALE) {
      case 10: ref_bc = 11.736328; break;
      case 16: ref_bc = 10.87493896; break;
      case 20: ref_bc = 10.52443173; break;
    }
    if (ref_bc != -1) {
      if ( fabs(avgbc - ref_bc) > 0.000001 ) {
        fprintf(stderr, "error: check failed: avgbc = %10.8g, ref = %10.8g\n", avgbc, ref_bc);
      }
    } else {
      printf("warning: no reference available\n");
    }

    printf("avg_centrality: %10.8g\n", avgbc);
    printf("centrality_time: %g\n", t); fflush(stdout);
    printf("centrality_teps: %g\n", (double)total_nedge / t);
  }
  
  //###################
  // Kernels complete!
  
  SoftXMT_merge_and_dump_stats();
  //SoftXMT_dump_stats_all_nodes();

  VLOG(1) << "freeing graphs";
  //free_graph(dirg);
  free_graph(g);
  VLOG(1) << "done freeing";
}

int main(int argc, char* argv[]) {
  SoftXMT_init(&argc, &argv, (1L<<34));
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
    }
  }
  
  // if no flags set, default to doing all
  if (!(do_components || do_pathiso || do_triangles || do_centrality)) {
    do_components = do_pathiso = do_triangles = do_centrality = true;
  }
}

