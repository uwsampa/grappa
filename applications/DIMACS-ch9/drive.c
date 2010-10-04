#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "graph.h"
#include "defs.h"
#include "utils.h"
#include "sp.h"

#ifdef __MTA__
#include <sys/mta_task.h>
#include <machine/runtime.h>
#else
#include <sys/time.h>
#endif
 
/* For generating the Cray Scale-free greaphs */
unsigned m8[] = { 1<<5, 1<<25, 1<<10, 1<<25, 1<<15, 1<<25, 1<<20, 1<<25, 1<<26, 1<<29, 100000000, 328911360};
unsigned m7[] = { 10, 100, 5, 100, 25, 100};
unsigned m6[] = { 2, 30, 20, 30 };
unsigned m5[] = {   1<< 2, 1<<8, 1<<12, 5<<12 };
unsigned m4[] = {   1<< 8, 1<<16, 1<<18, 5<<16 };
unsigned m3[] = { 1<<0, 1<<18, 1<< 8, 1<<18, 1<<18, 5<<18 };
unsigned m2[] = { 1<<0, 1<<19, 1<< 9, 1<<19, 1<<19, 5<<19 };
unsigned m1[] = { 1<<0, 1<<20, 1<<10, 1<<20, 1<<20, 5<<20 };
unsigned v0[] = { 1<<1, 1<<21, 1<<11, 1<<21, 1<<21, 5<<21 };
unsigned v1[] = { 1<<2, 1<<22, 1<<12, 1<<22, 1<<22, 5<<22 };
unsigned v2[] = { 1<<3, 1<<23, 1<<13, 1<<23, 1<<23, 5<<23 };
unsigned v3[] = { 1<<4, 1<<24, 1<<14, 1<<24, 1<<24, 5<<24 };
unsigned v4[] = { 1<<5, 1<<25, 1<<15, 1<<25, 1<<25, 5<<25 };
unsigned v5[] = { 1<<6, 1<<26, 1<<16, 1<<26, 1<<26, 5<<26 };
unsigned v6[] = { 1<<7, 1<<27, 1<<17, 1<<27, 1<<27, 5<<27 };
unsigned *v[] = {m8, m5, m4, m3, m2, m1, v0, v1, v2, v3, v4, v5, v6 };

int main(int argc, char *argv[]) 
{
	graph G;
	int nSrcs, trials;
	double start_time, end_time, *one_src_times, *all_src_times;
	int *srcs, *dests;
	int* int_weights;
	double* real_weights;

	int i, j, s, t;

#ifdef __MTA__
	int procs, mhz;
	double freq;
	int traps, issues, streams, ready, m_nops, a_nops, fp_ops, fp_nops, concurrency, phantoms, memrefs;
	int ticks, spticks;
	int perfCount;
#else
	struct timeval tp;
#endif

	int n, m;

    int levels;
    int problem;
    int curArgIndex = 1;
    int algCount = 0;
    int graph_type = -1;
    int weight_type = 1; /* default: real weights */
    int weight_dist = 0; /* default: uniform random weights */
    int undirected = 1;
	int max_weight = 100;
    int SCALE, d;
    double alpha, beta;
    int x, y, z, orbits;
    char **algs = (char**) malloc(sizeof(char*)*MAX_ALGS);
    char *filename = (char *) malloc(200*sizeof(char));
    int src = -1;
    char *desc = (char *) malloc(30*sizeof(char));
#ifdef __MTA__
	mta_suspend_event_logging();
	mta_reserve_task_event_counter(3, (CountSource) RT_FLOAT_TOTAL);
	mta_reserve_task_event_counter(2, (CountSource) RT_M_NOP);
	mta_reserve_task_event_counter(1, (CountSource) RT_TRAP);
	mta_set_trace_limit(1000000000000);
	procs = mta_nprocs();
	freq = (double) mta_clock_freq();
	mhz = freq/1000000;
	perfCount = 0;
#endif

    if (argc < 7) {
        fprintf(stderr,"usage:\n\tshortest_paths (-bfs | -st | -ds | -bf)"
                " -nSrcs <no. of source vertices/st pairs> "
                " -trials <no. of trials from each src vertex/st pair> "
                " -graph <graph class: rand, cray, rmat, grid,"
                " 2Dtor, 3Dtor, bullseye, dreg, hier, btree, randtree," 
                " geom, file> (-directed | -undirected)"
                " -weight <int, real, unit> -weight_dist"
                " <rand, polylog> -max_wt <SCALE>"
                " -graph_args <graph gen args>\n\n");
        fprintf(stderr,"Note: The max int weight is set to 2^SCALE\n");
        fprintf(stderr,"graph gen args:\n");
        fprintf(stderr,"\tcray: <levels> <graph no. (0 .. 15)>\n");
        fprintf(stderr,"\tplod: <SCALE> <alpha> <beta>\n");	
        fprintf(stderr,"\tgrid: <x> <y> (creates a grid of size 2^x by 2^y)\n");
        fprintf(stderr,"\t2Dtor: <x> <y> <d> (n=2^x * 2^y, m = n*d\n");
        fprintf(stderr,"\t3Dtor: <x> <y> <z> <d>\n");
        fprintf(stderr,"\tbullseye: <SCALE> <d> <orbits> (n = 2^SCALE)\n");
        fprintf(stderr,"\tothers: <int SCALE> <int d> (n = 2^SCALE, m = d*n)\n");
        fprintf(stderr,"\tfile: <filename>\n");
        fprintf(stderr, "\n");
        exit(1);
    }

    while (curArgIndex < argc) {
        if (strcmp(argv[curArgIndex],"-bfs")==0) {
            algs[algCount] = (char*) malloc(sizeof(char)*4);
            strcpy(algs[algCount++], "bfs");
        } else if (strcmp(argv[curArgIndex],"-st")==0) {
            algs[algCount] = (char*) malloc(sizeof(char)*3);
            strcpy(algs[algCount++], "st");
        } else if (strcmp(argv[curArgIndex],"-ds")==0) {
            algs[algCount] = (char*) malloc(sizeof(char)*3);
            strcpy(algs[algCount++], "ds");
        } else if (strcmp(argv[curArgIndex],"-bf")==0) {
            algs[algCount] = (char*) malloc(sizeof(char)*3);
            strcpy(algs[algCount++], "bf");
        } else if (strcmp(argv[curArgIndex],"-nSrcs")==0) {
            nSrcs = atoi(argv[++curArgIndex]);
        } else if (strcmp(argv[curArgIndex], "-trials")==0) {
            trials = atoi(argv[++curArgIndex]);
        } else if (strcmp(argv[curArgIndex],"-graph")==0) {
            curArgIndex++;
            if (strcmp(argv[curArgIndex], "rand") == 0)
                graph_type = 0;
            if (strcmp(argv[curArgIndex], "cray") == 0)
                graph_type = 1;
            if (strcmp(argv[curArgIndex], "rmat") == 0) 
                graph_type = 2;
            if (strcmp(argv[curArgIndex], "grid") == 0)
                graph_type = 4;
            if (strcmp(argv[curArgIndex], "2Dtor") == 0) 
                graph_type = 5;
            if (strcmp(argv[curArgIndex], "3Dtor") == 0) 
                graph_type = 6;
            if (strcmp(argv[curArgIndex], "bullseye") == 0) 
                graph_type = 7;
            if (strcmp(argv[curArgIndex], "dreg") == 0) 
                graph_type = 8;
            if (strcmp(argv[curArgIndex], "hier") == 0) 
                graph_type = 9;
            if (strcmp(argv[curArgIndex], "btree") == 0) 
                graph_type = 10;
            if (strcmp(argv[curArgIndex], "randtree") == 0) 
                graph_type = 11;
            if (strcmp(argv[curArgIndex], "geom") == 0) 
                graph_type = 12;
            if (strcmp(argv[curArgIndex], "file") == 0)
                graph_type = 13;
            if (graph_type == -1) {
                fprintf(stderr, "Error: Invalid graph type\n");
                exit(1);
            }
        } else if (strcmp(argv[curArgIndex],"-graph_args")==0) {
            curArgIndex++;
            if (graph_type == 0) {
                SCALE = atoi(argv[curArgIndex++]);
                d = atoi(argv[curArgIndex]);
            } else if (graph_type == 1) {
                levels = atoi(argv[curArgIndex++]);
                problem = atoi(argv[curArgIndex]);
            } else if (graph_type == 2) {
                SCALE = atoi(argv[curArgIndex++]);
                d = atoi(argv[curArgIndex]);
            } else if (graph_type == 4) {
                x = atoi(argv[curArgIndex++]);
                y = atoi(argv[curArgIndex]);
            } else if (graph_type == 5) {
                x = atoi(argv[curArgIndex++]);
                y = atoi(argv[curArgIndex++]);
                d = atoi(argv[curArgIndex]);
            } else if (graph_type == 6) {
                x = atoi(argv[curArgIndex++]);
                y = atoi(argv[curArgIndex++]);
                z = atoi(argv[curArgIndex++]);
                d = atoi(argv[curArgIndex]);
            } else if (graph_type == 7) {
                SCALE = atoi(argv[curArgIndex++]);
                d = atoi(argv[curArgIndex++]);
                orbits = atoi(argv[curArgIndex]);
            } else if ((graph_type >= 8) && (graph_type <= 12)) {
                SCALE = atoi(argv[curArgIndex++]);
                d = atoi(argv[curArgIndex]); 
            } else if (graph_type == 13) {
                strcpy(filename, argv[curArgIndex]);
            } else if (graph_type == -1) {
                fprintf(stderr, "Error: Specify graph class before the arguments\n");
            }
        } else if (strcmp(argv[curArgIndex],"-weight")==0) {
            curArgIndex++;
            if (strcmp(argv[curArgIndex], "unit") == 0)
                weight_type = 0;
            else if (strcmp(argv[curArgIndex], "real") == 0)
                weight_type = 1; 
            else if (strcmp(argv[curArgIndex], "int") == 0)
                weight_type = 2;
        } else if (strcmp(argv[curArgIndex], "-weight_dist")==0) {
            curArgIndex++;
            if (strcmp(argv[curArgIndex],"rand")==0) 
                weight_dist = 0;
            if (strcmp(argv[curArgIndex],"polylog")==0) 
                weight_dist = 1;
        } else if (strcmp(argv[curArgIndex],"-max_wt")==0) {
            curArgIndex++;
            max_weight = atoi(argv[curArgIndex]); 
            assert(max_weight > 0);
            assert(max_weight < 32);
            max_weight = 1<<max_weight;
        } else if (strcmp(argv[curArgIndex],"-directed")==0) {
            undirected = 0;
        } else if (strcmp(argv[curArgIndex],"-undirected")==0) {
            undirected = 1;
        }
        curArgIndex++;
    }
    if (algCount == 0) {
        fprintf(stderr, "Error: No algorithm selected\n");
        exit(1); 
    }

    if (trials == 0) {
        fprintf(stderr, "Error: Set the number of trials\n");
        exit(1);
    } 

    if (nSrcs == 0) {
        fprintf(stderr, "Error: Set the number of source vertices\n");
        exit(1);
    }

    /* generate graph and edge weights */

    if (graph_type == 0) {
        n = 1<<SCALE;
        m = n*d;
        random_Gnm(n, m, weight_type, weight_dist, max_weight, &srcs, &dests, 
            &int_weights, &real_weights, undirected);
    } else if (graph_type == 1) {
        CraySF(levels, v[problem], &n, &m, weight_type, weight_dist, max_weight, &srcs, &dests, 
            &int_weights, &real_weights, undirected); 
    } else if (graph_type == 2) {
        n = 1<<SCALE;
        m = d*n; 
        RMatSF(SCALE, d, weight_type, weight_dist, max_weight, &srcs, &dests,
                &int_weights, &real_weights, undirected);
    } else if (graph_type == 3) {
    } else if (graph_type == 4) {
       grid(x, y, &n, &m, weight_type, weight_dist, max_weight, &srcs, &dests,
               &int_weights, &real_weights, undirected); 
    } else if (graph_type == 5) {
    } else if (graph_type == 6) {
    } else if (graph_type == 7) {
    } else if (graph_type == 8) {
    } else if (graph_type == 9) {
    } else if (graph_type == 10) {
    } else if (graph_type == 11) {
    } else if (graph_type == 12) {
    } else if (graph_type == 13) {
        weight_type = 2;
        max_weight = -1;
        undirected = 0;
        readGraphFromFile(filename, &src, desc, &n, &m, weight_type,
                &max_weight, &srcs, &dests, &int_weights, &real_weights,
                undirected);
        printf("DIMACS input file: %s\n", filename);
    } 

	/* load the graph into G */
	loadGraph(&G, weight_type, n, m, srcs, dests, int_weights, real_weights,
            undirected);	
   
    fprintf(stderr, "nodes: %d, edges: %d\n", G.n, G.m);

	/* Free temp graph structures */
	free(srcs);
	free(dests);

	one_src_times = (double *) malloc(trials*sizeof(double));
	all_src_times = (double *) malloc(nSrcs*sizeof(double));
	srand48(3324545);

	for (i=0; i<nSrcs; i++) {

		s = ((int) lrand48()) % G.n;
		t = ((int) lrand48()) % G.n;
 	
		for (j=0; j<trials; j++) {

#ifdef __MTA__
            /* get the counters for the first two trials */
			if (perfCount < 2) {
				mta_resume_event_logging();
				issues = mta_get_task_counter(RT_ISSUES);
				memrefs= mta_get_task_counter(RT_MEMREFS);
				streams = mta_get_task_counter(RT_STREAMS);
				concurrency = mta_get_task_counter(RT_CONCURRENCY);
				phantoms = mta_get_team_counter(RT_PHANTOM);	
				ready = mta_get_team_counter(RT_READY);	
				m_nops = mta_get_task_counter(RT_M_NOP);
				a_nops = mta_get_task_counter(RT_M_NOP);
				fp_ops = mta_get_task_counter(RT_FLOAT_TOTAL);
				traps = mta_get_task_counter(RT_TRAP);
			}
#endif

            if (find(algs, algCount, "st")) {
				one_src_times[j] = P2P(&G, s, t);
            } else if (find(algs, algCount, "bfs")) {
                fprintf(stderr, "Running BFS\n");
                one_src_times[j] = BFS(&G, s);
            } else { 
                if (find(algs, algCount, "ds"))
				    one_src_times[j] = NSSSP(&G, s, weight_type, i*trials+j, 0);
                else
                    one_src_times[j] = NSSSP(&G, s, weight_type, i*trials+j, 1);
            }

#ifdef __MTA__

			/* Get the performance counters for a couple of runs */
			if (perfCount < 2) {	
				perfCount++;
				issues = mta_get_task_counter(RT_ISSUES) - issues;
				memrefs= mta_get_task_counter(RT_MEMREFS) - memrefs;
				streams = mta_get_task_counter(RT_STREAMS) - streams;
				concurrency = mta_get_task_counter(RT_CONCURRENCY) - concurrency;
				phantoms = mta_get_team_counter(RT_PHANTOM) - phantoms;
				ready = mta_get_team_counter(RT_READY) - ready;
				fp_ops = mta_get_task_counter(RT_FLOAT_TOTAL) - fp_ops;
				m_nops = mta_get_task_counter(RT_M_NOP)  - m_nops;
				a_nops = mta_get_task_counter(RT_A_NOP)  - a_nops;
				traps = mta_get_task_counter(RT_TRAP) - traps; 
				mta_suspend_event_logging();
				fprintf(stderr, "issues: %d, memrefs: %d, streams: %lf, fp_ops: %d, eff A issues: %lf, perc float issues: %lf, perc phantoms: %lf, perc concurrency: %lf, ready: %lf, memratio:%lf, traps: %d\n",
                    issues, memrefs, streams/(double)issues, fp_ops, 
                 	((double)(issues - a_nops - m_nops))/(issues), ((double)(fp_ops))/issues, 
                    phantoms/(double)issues, concurrency/(double)issues,
                    ready/(double)issues, memrefs/(double)issues, traps);
			}
#endif

		}

		storeTimes(one_src_times, all_src_times, i, trials);

	}

	printTimingStats(all_src_times, nSrcs);

	free(one_src_times);
	free(all_src_times);
	
	free(G.numEdges);
	free(G.endV);
    
    if (G.realWt)
	    free(G.realWt);
    free(desc);
    free(filename);
    for (i=0; i<algCount; i++)
        free(algs[i]);
    free(algs);
    
	return 0;

}
