#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
// #include <sys/resource.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>

#ifdef __MTA__
#include <sys/mta_task.h>
#include <machine/runtime.h>
#else
#include "compat/xmt-ops.h"
#endif

#include "globals.h"
#include "defs.h"
#include "xmt-luc.h"
#include "luc_file_io/luc_file_io.h"
#include "stinger-atomics.h"


int64_t isint(char * buffer)
{
	while(*buffer != 0)
	{
		if(*buffer >= '0' && *buffer <= '9')
			buffer++;
		else return 0;
	}
	return 1;
}

void bfs(graph * G, int64_t v)
{
		int64_t NV = G->numVertices;
		int64_t * Q = xmalloc(sizeof(int64_t)*NV);
		int64_t * dist = xmalloc(sizeof(int64_t)*NV);
		Q[0] = v;
		int64_t Qnext = 1;

		int64_t nQ = 1;

		int64_t * marks = G->marks;
		int64_t * start = G->startVertex;
		int64_t * eV = G->endVertex;
		int64_t d_phase;
		int64_t Qstart = 0;
		int64_t Qend = 1;
		int64_t j, k;

		for(j=0; j<NV; j++)
			marks[j] = 0;
		
		marks[v] = 1;
PushOnStack:    /* Push nodes onto Q */
		d_phase = nQ;

MTA("mta assert no dependence")
MTA("mta block dynamic schedule")
MTA("mta use 100 streams")
		for (j = Qstart; j < Qend; j++) {
			int64_t v = Q[j];
			int64_t myStart = start[v];     
			int64_t myEnd   = start[v+1];
			for (k = myStart; k < myEnd; k++) {
				int64_t d, w;
				w = eV[k];                    
				d = dist[w]; 
				if (d < 0) {
					if (stinger_int_fetch_add(marks + w, 1) == 0) {
						dist[w] = d_phase; Q[stinger_int64_fetch_add(&Qnext, 1)] = w;
						
					}
				}
			}
		}

		Qstart = Qend;
		Qend = Qnext;

		if(Qstart != Qend)
			goto PushOnStack;
}

void scriptCheck ( char * scriptname, int64_t run)
{
	FILE * sfile = fopen(scriptname, "r");
	char buffer[1000];
	char secondary[1000];
	int64_t opened = 0;
	int64_t saved = 0;
	int64_t writeable = 0;
	int64_t kernel_id = 0;

	graph * G = NULL;
	graph * S = NULL;
	double * result = NULL;
	int64_t NV, NE;
	int64_t extract = 0;
	int64_t max, maxV;

	while(fscanf(sfile, "%s", buffer) != EOF)
	{
		if(!opened && strcmp(buffer, "read") != 0)
		{
			printf("SCRIPT ERROR: first command must be 'read'.\n");
			exit(1);
		}

		if(strcmp(buffer, "modularity") != 0 && 
			strcmp(buffer, "conductance") != 0 && extract == 1)
		{
			printf("Generating subgraph...");
			graph * G2 = genSubGraph(G, NV, 1);
			printf("done\n");
			if(G != S)
				free_graph(G);
			G = G2;
			extract = 0;
		}
		
		if(strcmp(buffer, "read") == 0)
		{
			fscanf(sfile, "%s", secondary);
			printf("trying to read file as %s...\n", secondary);
			if(G != S)
				free_graph(G);	
			printf("done free graph\n");
			if(strcmp(secondary, "dimacs")==0)
			{
				fscanf(sfile, "%s", secondary);
				G = parse_DIMACS_graph(secondary);
			}
			else if(strcmp(secondary, "binary")==0)
			{
				printf("reading binary graph sizeof(graph) = %ld\n", sizeof(graph));
				fscanf(sfile, "%s", secondary);
				G = (graph *) xmalloc(sizeof(graph));
				printf("init graphio...\n");
				graphio_b(G, secondary, 4);
				printf("done\n");
			}
			else if(strcmp(secondary, "el")==0)
			{
				printf("reading binary edge list graph sizeof(graph) = %ld\n", sizeof(graph));
				fscanf(sfile, "%s", secondary);
				G = (graph *) xmalloc(sizeof(graph));
				printf("init graphio...\n");
				graphio_el(G, secondary);
				printf("done\n");
			}
			else
			{
				printf("SYNTAX ERROR: '%s' is not a valid graph type to read.\n", secondary);
				exit(1);
			}
			opened = 1;
			NV = G->numVertices;
			NE = G->numEdges;
			printf("NV: %ld, NE: %ld\n", NV, NE);
			result = xmalloc(sizeof(double)*NV);
			graphCheck(G);
			printf("Estimating graph diameter using 128 sources...\n");
			fflush(stdout);
			double time = timer();
			SCALE = NV >> 4;
			stats_tic ("estdiameter");
			int64_t est_diameter = calculateGraphDiameter(G, 128);
			stats_toc ();
			SCALE = est_diameter << 2;
			getUserParameters(SCALE, 8);
			time = timer() - time;
			printf("Estimated graph diameter: %ld\n", est_diameter);
			printf("Setting SCALE appropriately.\n");
			printf("Time taken is %9.6lf sec.\n", time);
			fflush(stdout);
		}
		else if(strcmp(buffer, "=>") == 0)
		{
			if(!writeable)
			{
				printf("SCRIPT ERROR: '=>' was not preceded by operation that produces writeable output.\n");
				exit(1);
			}

			fscanf(sfile, "%s", secondary);

			if(writeable == 2)
			{
				fwrite_doubles(result, G->numVertices, secondary);
			}
			else
			{
				size_t filesize = 3+G->numVertices+2*G->numEdges;
				uint32_t * output = xmalloc(sizeof(uint32_t)*filesize);
				output[0] = (uint32_t) G->numEdges;
				output[1] = (uint32_t) G->numVertices;
				for(int64_t i=0; i<G->numVertices; i++)
				{
					output[3+i] = (uint32_t) G->edgeStart[i];
				}
				for(int64_t i=0; i<G->numEdges; i++)
				{
					output[3+G->numVertices+i] = (uint32_t) G->endVertex[i];
					output[3+G->numVertices+numEdges+i] = (uint32_t) G->intWeight[i];
				}

//				luc_fwrite(output, filesize, sizeof(uint32_t), 0, secondary);
				xmt_luc_snapout(secondary, output, filesize*sizeof(uint32_t));
				
				free(output);
			}
		}
		
		writeable = 0;
		
		if(strcmp(buffer, "read") == 0);
		else if(strcmp(buffer, "=>") == 0);
		else if(strcmp(buffer, "restore") == 0)
		{
			if(!saved)
			{
				printf("SCRIPT ERROR: 'restore' command before any graph 'save'.\n");
				exit(1);
			}
			if(G != S)
				free_graph(G);
			G = S;
			writeable = 1;
		}
		else if(strcmp(buffer, "save") == 0)
		{
			free_graph(S);
			S = G;
			saved = 1;
			writeable = 1;
		}
		else if(strcmp(buffer, "extract") == 0)
		{
			extract = 1;
			writeable = 1;
			fscanf(sfile, "%s", secondary);
			if(strcmp(secondary, "bfs")== 0)
			{
				fscanf(sfile, "%s", secondary);
				if(!isint(secondary))
				{
					printf("SYNTAX ERROR: 'bfs' extraction requires numeric argument\n");
					exit(1);
				}

				bfs(G, atoi(secondary));
			}
			else if(strcmp(secondary, "kcore") == 0)
			{
				fscanf(sfile, "%s", secondary);
				if(!isint(secondary))
				{
					printf("SYNTAX ERROR: 'kcore' extraction requires numeric argument\n");
					exit(1);
				}
				
				kcore(G, atoi(secondary));
				int64_t i;
				for(i=0; i<NV; i++)
				{
					G->marks[i] = (G->marks[i] > 0);
				}
			}
			else if(strcmp(secondary, "component") == 0)
			{
				fscanf(sfile, "%s", secondary);
				if(!isint(secondary))
				{
					printf("SYNTAX ERROR: 'component' extraction requires numeric argument\n");
					exit(1);
				}
				
				connectedComponents(G);
				int64_t rank = atoi(secondary);
				int64_t i;
				
				int64_t * D = G->marks;
				int64_t * T = xmalloc (NV * sizeof(int64_t));
				for (i = 0; i < NV; i++) {
					T[i] = 0;
				}

MTA("mta assert no dependence")
				for (i = 0; i < NV; i++) {
					stinger_int64_fetch_add(&T[D[i]], 1);
				}

				int64_t argmax;
				for (i = 0; i<rank; i++)
				{
					int64_t max = 0;
					int64_t j;
					for( j=0; j<NV; j++)
					{
						if(T[j] > max)
						{
							max = T[j];
							argmax = j;
						}
					}
					T[argmax] = 0;
				}

				printf("extracted component number %ld\n\n", argmax);

				for (i = 0; i<NV; i++)
					if(D[i] == argmax)
						D[i] = 1;
					else
						D[i] = 0;
			}
		}
		else if (strcmp(buffer, "kcentrality")==0)
		{
			writeable = 2;
			kernel_id = 1;
			fscanf(sfile, "%s", secondary);
			if(!isint(secondary))
			{
				printf("SYNTAX ERROR: 'kcentrality' kernel requires (2) numeric arguments\n");
				exit(1);
			}
			int64_t k = atoi(secondary);
			fscanf(sfile, "%s", secondary);
			if(!isint(secondary))
			{
				printf("SYNTAX ERROR: 'kcore' extraction requires (2) numeric arguments\n");
				exit(1);
			}
			int64_t v0 = atoi(secondary);

			if(result != NULL)
			{
				free(result);
				result = xmalloc(sizeof(double)*NV);
			}

			printf("\nkcentrality(%ld) beginning execution...\n", k);
			fflush (stdout);

			stats_tic ("kcentrality");
			double timeBC = kcentrality(G, result, v0, k);
			stats_toc ();

			printf("\tkcentrality() completed execution.\n");

			printf("Maximum kBC Vertices\n");
			printf("--------------------\n");
			int64_t j;
			double maxBC;
			int64_t maxI;
			for (j = 0; j < 10; j++) {
				maxI  = 0;
				maxBC = result[0];
				int64_t i;
				
				for (i = 1; i < NV; i++) if (result[i] > maxBC) {maxBC = result[i]; maxI = i;}
				printf("#%2ld: %8ld - %20.15e\n", j+1, maxI, maxBC);
				result[maxI] = -1.0;
			}

			printf("Time taken is %9.6lf sec.\n\n", timeBC);
		}
		else if (strcmp(buffer, "modularity")==0)
		{
			kernel_id = 2;
			printf("\nCalculating the modularity score of the graph...\n");
			fflush (stdout);
			double time = timer();
			stats_tic ("modularity");
			computeModularityValue(G, G->marks, NV);
			stats_toc ();
			time = timer() - time;
			printf("Time taken is %9.6lf sec.\n\n", time);
			
		}
		else if (strcmp(buffer, "degree")==0)
		{
			kernel_id = 3;
			printf("\nCalculating out-degree distributions...\n");
			fflush (stdout);
			double time = timer();
			stats_tic ("degreedist");
			calculateDegreeDistributions(G);
			stats_toc ();
			time = timer() - time;
			printf("Time taken is %9.6lf sec.\n\n", time);
		}
		else if (strcmp(buffer, "conductance")==0)
		{
			kernel_id = 4;
			printf("Calculating conductance of the largest connected component...\n");
			fflush (stdout);
			double time = timer();
			stats_tic ("conductance");
			computeConductanceValue(G, G->marks);
			stats_toc ();
			time = timer() - time;
			printf("Time taken is %9.6lf sec.\n\n", time);
		}
		else if (strcmp(buffer, "components")==0)
		{
			kernel_id = 5;
			double time = timer();
			int64_t numComp = connectedComponents(G);
			time = timer() - time;
			printf("There are %ld components in the graph.\n", numComp);
			calculateComponentDistributions(G, numComp, &max, &maxV);
			printf("Time taken is %9.6lf sec.\n\n", time);
		}
		else if (strcmp(buffer, "clustering")==0)
		{
			writeable = 2;
			kernel_id = 6;
			if(result != NULL)
				free(result);
			printf("\nCalculating local clustering coefficients...\n");
			fflush (stdout);
			double time = timer();
			stats_tic ("clusteringcoeff");
			result = calculateClusteringLocal(G);
			stats_toc ();
			time = timer() - time;
			printf("Time taken is %9.6lf sec.\n\n", time);

		}
		else if (strcmp(buffer, "transitivity")==0)
		{
			writeable = 2;
			kernel_id = 7;
			if(result != NULL)
				free(result);
			printf("\nCalculating the global transitivity coefficient...\n");
			fflush (stdout);
			double time = timer();
			stats_tic ("transitivity");
			result = calculateTransitivityLocal(G);
			stats_toc ();
			time = timer() - time;
			printf("Time taken is %9.6lf sec.\n\n", time);
		}
		else if (strcmp(buffer, "diameter")==0)
		{
			kernel_id = 8;
			fscanf(sfile, "%s", secondary);
			if(!isint(secondary))
			{
				printf("SYNTAX ERROR: 'diameter' kernel requires numeric argument\n");
				exit(1);
			}
			int64_t Vs = atoi(secondary);
			printf("\nCalculating graph diameter...\n");
			fflush (stdout);
			double time = timer();
			stats_tic ("diameter");
			calculateGraphDiameter(G, Vs);
			stats_toc ();
			time = timer() - time;
			printf("Time taken is %9.6lf sec.\n\n", time);
		} else if (strcmp (buffer, "printstats") == 0) {
		  print_stats ();
		} else {
			printf("SYNTAX ERROR: %s is not a recognized command.\n", buffer);
		}
	}
}
