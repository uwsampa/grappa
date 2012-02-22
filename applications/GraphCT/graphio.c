#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include "defs.h"
#include "globals.h"
#include "read-graph-el.h"
#include "xmt-luc.h"
#include "luc_file_io/luc_file_io.h"

int64_t graphio_b(graph *G, char *filename, int64_t mode) {
	int64_t i;
	int64_t NE, NV;
	size_t sz;

	xmt_luc_io_init();
	xmt_luc_stat (filename, &sz);

	if (mode == 4)
	{
		uint32_t * input_graph = (uint32_t *) xmalloc (sz);
		xmt_luc_snapin (filename, input_graph, sz);

		NE = input_graph[0];
		NV = input_graph[1];

		alloc_graph (G, NV, NE);

		uint32_t * start = input_graph + 3;
		uint32_t * eV = input_graph + 3 + NV;
		uint32_t * weight = input_graph + 3 + NV + NE;

MTA("mta assert nodep")
		for(i = 0; i<NE; i++)
		{
#ifdef __MTA__
			G->endVertex[i] = (int64_t) ntohl (eV[i]);
			G->intWeight[i] = (int64_t) ntohl (weight[i]);
#else
			G->endVertex[i] = (int64_t) eV[i];
			G->intWeight[i] = (int64_t) weight[i];
#endif
		}

MTA("mta assert nodep")
		for(i = 0; i<NV; i++)
		{
#ifdef __MTA__
			G->edgeStart[i] = (int64_t) ntohl (start[i]);
#else
			G->edgeStart[i] = (int64_t) start[i];
#endif
			G->marks[i] = 0;
		}

		G->edgeStart[NV] = NE;

MTA("mta assert nodep")
		for(i = 0; i<NV; i++) {
			int64_t k;
			int64_t myStart = G->edgeStart[i];
			int64_t myEnd = G->edgeStart[i+1];
MTA("mta assert nodep")
			for (k = myStart; k < myEnd; ++k)
				G->startVertex[k] = i;
		}

		free(input_graph);
	}
	else if (mode == 8)
	{
		uint64_t * input_graph = xmalloc (sz);
		xmt_luc_snapin (filename, input_graph, sz);

		NE = input_graph[0];
		NV = input_graph[1];

		alloc_graph (G, NV, NE);

		uint64_t * start = input_graph + 3;
		uint64_t * eV = input_graph + 3 + NV;
		uint64_t * weight = input_graph + 3 + NV + NE;

MTA("mta assert nodep")
		for(i = 0; i<NE; i++)
		{
#ifdef __MTA__
			G->endVertex[i] = (int64_t) ntohl (eV[i]);
			G->intWeight[i] = (int64_t) ntohl (weight[i]);
#else
			G->endVertex[i] = (int64_t) eV[i];
			G->intWeight[i] = (int64_t) weight[i];
#endif
		}

MTA("mta assert nodep")
		for(i = 0; i<NV; i++)
		{
#ifdef __MTA__
			G->edgeStart[i] = (int64_t) ntohl (start[i]);
#else
			G->edgeStart[i] = (int64_t) start[i];
#endif
			G->marks[i] = 0;
		}

		G->edgeStart[NV] = NE;

MTA("mta assert nodep")
		for(i = 0; i<NV; i++) {
			int64_t k;
			int64_t myStart = G->edgeStart[i];
			int64_t myEnd = G->edgeStart[i+1];
MTA("mta assert nodep")
			for (k = myStart; k < myEnd; ++k)
				G->startVertex[k] = i;
		}

		free(input_graph);
	}

	return 0;
}

int64_t graphio_el(graph *G, char *filename) {
	int64_t i;
	int64_t NE, ne2;

	struct el g;
	graphSDG tmp;

	read_graph_el (filename, &g);

	{
	  DECL_EL(g);

	  NE = 2*g.ne;
	  for (i = 0; i < g.nv; ++i)
	    if (D(g,i) > 0) ++NE;

	  alloc_edgelist (&tmp, NE);
	  ne2 = 0;
	  MTA("mta assert nodep")
	    for (i = 0; i < g.nv; ++i)
	      if (D(g,i) > 0) {
		int64_t k = ne2++;
		tmp.startVertex[k] = i;
		tmp.endVertex[k] = i;
		tmp.intWeight[k] = D(g,i);
	      }

	  MTA("mta assert nodep")
	    for (i = 0; i < g.ne; ++i) {
	      int64_t k1 = ne2++;
	      int64_t k2 = ne2++;
	      tmp.startVertex[k1] = I(g,i);
	      tmp.endVertex[k1] = J(g,i);
	      tmp.intWeight[k1] = W(g,i);
	      tmp.startVertex[k2] = J(g,i);
	      tmp.endVertex[k2] = I(g,i);
	      tmp.intWeight[k2] = W(g,i);
	    }
	}

	free_graph_el (&g);

	computeGraph(G, &tmp);

	free_edgelist (&tmp);
	return 0;
}

int64_t fwrite_doubles(double *input, int64_t num, char *filename)
{
	xmt_luc_io_init();

	char *output = xmalloc (num * 24 * sizeof(char));

	int64_t i;
MTA("mta assert parallel")
	for (i = 0; i < num; i++)
	{
		sprintf(output+24*i, "%23.15e\n", input[i]);
	}
MTA("mta assert parallel")
	for (i = 0; i < num * 24; i++)
	{
		if (output[i] == '\0') output[i] = ' ';
	}

//	remove(filename);
	
//	luc_fwrite(output, 1, num * 24 * sizeof(char), 0, filename);
	xmt_luc_snapout(filename, output, num * 24 * sizeof(char));

//	free(output);

	return 1;
}

int64_t fwrite_ints(int64_t *input, int64_t num, char *filename)
{
	char *output = xmalloc (num * 11 * sizeof(char));

	int64_t i;
MTA("mta assert parallel")
	for (i = 0; i < num; i++)
	{
		sprintf(output+11*i, "%10ld\n", input[i]);
	}
MTA("mta assert parallel")
	for (i = 0; i < num * 11; i++)
	{
		if (output[i] == '\0') output[i] = ' ';
	}

//	remove(filename);
	
//	luc_fwrite(output, 1, num * 11 * sizeof(char), 0, filename);
	xmt_luc_snapout(filename, output, num * 11 *sizeof(char));

//	free(output);

	return 1;
}

void graphCheck(graph *G)
{
	int64_t NV = G->numVertices;
	int64_t NE = G->numEdges;
	int64_t * start = G->edgeStart;
	int64_t * sV = G->startVertex;
	int64_t * eV = G->endVertex;
	int64_t errors = 0;

	printf("NV: %ld\nNE: %ld\n", NV, NE);

	for (int64_t k = 0; k < NV; k++)
	{
		if (start[k] < 0) errors++;
		if (start[k] > NE) errors++;
	}

	if (errors)
	{
		printf("Error encountered in edgeStart[]!\n");
		exit(-1);
	}

	errors = 0;

	for (int64_t k = 0; k < NV; k++)
	{
		if (sV[k] < 0) errors++;
		if (sV[k] >= NV) errors++;
	}

	if (errors)
	{
		printf("Error encountered in startVertex[]!\n");
		exit(-1);
	}

	errors = 0;

	for (int64_t k = 0; k < NV; k++)
	{
		if (eV[k] < 0) errors++;
		if (eV[k] >= NV) errors++;
	}

	if (errors)
	{
		printf("Error encountered in endVertex[]!\n");
		exit(-1);
	}
}

