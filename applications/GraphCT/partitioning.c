#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "defs.h"
#include "globals.h"
#ifdef __MTA__
#include <sys/mta_task.h>
#include <machine/runtime.h>
#else
#include "compat/xmt-ops.h"
#endif
#include <math.h>
#include <sys/mman.h>

#define HEAVYEDGE 1
#define CNM 2
#define MB 3
#define CONDUCTANCE 4
#define TIME

int64_t *global_ptr;

struct graph_part {
	int64_t NV, NE;
	int64_t *xsrc;
	int64_t *xadj;
	int64_t *xoff;
	int64_t *xoffend;
	int64_t *vweight;
	int64_t *eweight;
};

void alloc_graph_part(struct graph_part *G, int64_t NV, int64_t NE)
{
	size_t bufsize;
	void *mem;

	bufsize = NE * sizeof(int64_t) +
				NE * sizeof(int64_t) +
				(NV+2) * sizeof(int64_t) +
				(NV+2) * sizeof(int64_t) +
				NV * sizeof(int64_t) +
				NE * sizeof(int64_t);
	mem = xmmap (NULL, bufsize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, 0, 0);

	G->NV = NV;
	G->NE = NE;
	G->xsrc = (int64_t *) mem;
	G->xadj = (int64_t *) &(G->xsrc[NE]);
	G->xoff = (int64_t *) &(G->xadj[NE]);
	G->xoffend = (int64_t *) &(G->xoff[NV+2]);
	G->vweight = (int64_t *) &(G->xoffend[NV+2]);
	G->eweight = (int64_t *) &(G->vweight[NV]);
}


int myWeightedCompare(const void *a, const void *b)
{
	int64_t i = *(int64_t*)a;
	int64_t j = *(int64_t*)b;

	if (global_ptr[i] < global_ptr[j]) return -1;
	if (global_ptr[i] == global_ptr[j]) return 0;
	else return 1;
}

void quickSort(int64_t *arr, int64_t *indices, int64_t left, int64_t right) 
{
      int64_t i = left, j = right;
      int64_t tmp;
      int64_t pivot = arr[indices[(left + right) / 2]];

      /* partition */
      while (i <= j) {
            while (arr[indices[i]] < pivot)
                  i++;
            while (arr[indices[j]] > pivot)
                  j--;
            if (i <= j) {
                  tmp = indices[i];
                  indices[i] = indices[j];
                  indices[j] = tmp;
                  i++;
                  j--;
            }
      };

      /* recursion */
      if (left < j)
            quickSort(arr, indices, left, j);
      if (i < right)
            quickSort(arr, indices, i, right);
}

void RobsDumbSort(int64_t *arr, int64_t *indices, int64_t left, int64_t right)
{
	/* this sort has some issues still */
	MTA("mta assert nodep")
	for (int64_t k = left; k <= right; k++)
	{
		int64_t v = arr[k];

		int64_t sum = 0;
		for (int64_t j = left; j <= right; j++)
		{
			if (arr[k] < v)
			{
				sum++;
			}
			else if (arr[k] == v)
			{
				if (k < j) sum++;
			}
		}

		indices[k] = left + sum;
	}
}

MTA("mta inline")
void insertionSort(int64_t *arr, int64_t *indices, int64_t left, int64_t right)
{
	int64_t i, j, s, t;

	for (i = left+1; i <= right; i++)
	{
		j = i;
		s = indices[j];
		t = arr[s];
		while (j > left && arr[indices[j-1]] > t)
		{
			indices[j] = indices[j-1];
			j--;
		}
		indices[j] = s;
	}
}

MTA("mta inline")
void oddEvenSort(int64_t *arr, int64_t *indices, int64_t left, int64_t right)
{
	int64_t sorted = 0;
	int64_t i;
	int64_t tmp;

	while (!sorted)
	{
		sorted = 1;
		MTA("mta assert nodep")
		for (i = left; i < right; i+=2)
		{
			if (arr[indices[i]] > arr[indices[i+1]])
			{
				tmp = indices[i];
				indices[i] = indices[i+1];
				indices[i+1] = tmp;
				sorted = 0;
			}
		}

		MTA("mta assert nodep")
		for (i = left+1; i < right; i+=2)
		{
			if (arr[indices[i]] > arr[indices[i+1]])
			{
				tmp = indices[i];
				indices[i] = indices[i+1];
				indices[i+1] = tmp;
				sorted = 0;
			}
		}
	}
}


#if 0
void SortStartDoubleW(int64_t NV, int64_t NE, int64_t *sv1, int64_t *ev1, int64_t *w1, int64_t *sv2, int64_t *ev2, int64_t *w2, int64_t *start)
{ 
  int64_t i;
  for (i = 0; i < NV + 2; i++) start[i] = 0;

  start += 2;

/* Histogram key values */
MTA("mta assert no alias *start *sv1")
  for (i = 0; i < NE; i++) start[sv1[i]] ++;

/* Compute start index of each bucket */
  for (i = 1; i < NV; i++) start[i] += start[i-1];

  start --;

/* Move edges into its bucket's segment */
MTA("mta assert no dependence")
  for (i = 0; i < NE; i++) {
      int64_t index = start[sv1[i]] ++;
      sv2[index] = sv1[i];
      ev2[index] = ev1[i];
      w2[index] = w1[i];
} }


void FindSortOffsets(int64_t NV, int64_t NE, int64_t *sv1, int64_t *start, int64_t *output)
{ 
  int64_t i;
  for (i = 0; i < NV + 2; i++) start[i] = 0;

  start += 2;

/* Histogram key values */
MTA("mta assert no alias *start *sv1")
  for (i = 0; i < NE; i++) start[sv1[i]] ++;

/* Compute start index of each bucket */
  for (i = 1; i < NV; i++) start[i] += start[i-1];

  start --;

/* Move edges into its bucket's segment */
MTA("mta assert no dependence")
  for (i = 0; i < NE; i++) {
      int64_t index = start[sv1[i]] ++;
	  output[index] = i;
} }
#endif

long double calcStdDeviation(double *score, int64_t n)
{
	long double sum = 0;
	long double sum2 = 0;
	long double average, variance, std_dev;
	for (int64_t k = 0; k < n; k++)
	{
		long double w = score[k];
		sum += w;
	}
	average = sum / n;
	for (int64_t k = 0; k < n; k++)
	{
		long double w = score[k];
		sum2 += (w - average) * (w - average);
	}
	variance = sum2 / (n-1);
	std_dev = sqrt(variance);

#ifdef DEBUG
	printf("average: %20.15e\nvariance: %20.15e\nstd_dev: %20.15e\n", average, variance, std_dev);
#endif

	return std_dev;
}


int64_t parallel_matching(int64_t NV, int64_t NE, int64_t *xoff, int64_t *xoffend, int64_t *xadj,
double *score, int64_t *matched, int64_t *pickedV, int64_t *pickedE)
{
	int64_t accum;

	OMP("omp parallel for")
	MTA("mta assert nodep")
	for (int64_t k = 0; k < NV; k++)
	{
		pickedV[k] = -1;
//		pickedE[k] = -1;
		matched[k] = -1;
	}

	OMP("omp parallel for")
	MTA("mta assert nodep")
	for (int64_t k = 0; k < NV; k++)
	{
		int64_t myStart = xoff[k];
		int64_t myEnd = xoffend[k];

		if (myStart == myEnd)
		{
			pickedV[k] = -k;
			continue;
		}

		while (myStart < myEnd)
		{
			if (xadj[myStart] != k) break;
			myStart++;
		}

		int64_t local_maxV = xadj[myStart];
		double local_max = score[myStart];
		int64_t local_maxE = myStart;

		for (int64_t j = myStart+1; j < myEnd; j++)
		{
			if (xadj[j] == k) continue;
			if (score[j] > local_max)
			{
				local_max = score[j];
				local_maxV = xadj[j];
				local_maxE = j;
			}
		}

		pickedV[k] = local_maxV;
//		pickedE[k] = local_maxE;
	}

	OMP("omp parallel for")
	MTA("mta assert nodep")
	for (int64_t k = 0; k < NV; k++)
	{
		int64_t myPick = pickedV[k];

		if (pickedV[myPick] == k)
		{
			matched[k] = myPick;
		}
	}

	accum = 0;
	MTA("mta assert nodep")
	for (int64_t k = 0; k < NV; k++)
	{
		if (matched[k] > -1) accum++;
	}

#ifdef DEBUG
	printf("accum: matched %ld vertices after 1 round\n", accum);
#endif

#define MAX_ITER 10
	int64_t iter = 1;
	int64_t accum_prev = -1;
	while (iter < MAX_ITER && accum_prev != accum)
	{
		accum_prev = accum;

		OMP("omp parallel for")
		MTA("mta assert nodep")
		for (int64_t k = 0; k < NV; k++)
		{
			pickedV[k] = -1;
//			pickedE[k] = -1;
		}

		OMP("omp parallel for")
		MTA("mta assert nodep")
		for (int64_t k = 0; k < NV; k++)
		{
			if (matched[k] == -1)
			{
				int64_t myStart = xoff[k];
				int64_t myEnd = xoffend[k];

				while (myStart < myEnd)
				{
					if (xadj[myStart] != k && matched[xadj[myStart]] == -1) break;
					myStart++;
				}

				int64_t local_maxV = xadj[myStart];
				double local_max = score[myStart];
				int64_t local_maxE = myStart;

				for (int64_t j = myStart+1; j < myEnd; j++)
				{
					if (xadj[j] == k || matched[xadj[j]] == -1) continue;
					if (score[j] > local_max)
					{
						local_max = score[j];
						local_maxV = xadj[j];
						local_maxE = j;
					}
				}

				pickedV[k] = local_maxV;
//				pickedE[k] = local_maxE;
			}
		}

		OMP("omp parallel for")
		MTA("mta assert nodep")
		for (int64_t k = 0; k < NV; k++)
		{
			int64_t myPick = pickedV[k];

			if (pickedV[myPick] == k)
			{
				matched[k] = myPick;
			}
		}

		accum = 0;
		MTA("mta assert nodep")
		for (int64_t k = 0; k < NV; k++)
		{
			if (matched[k] > -1) accum++;
		}

#ifdef DEBUG
		printf("accum: matched %ld vertices after %ld rounds\n", accum, iter+1);
#endif
		iter++;

	}

	return accum;
}


void calcScore(int64_t NV, int64_t NE, int64_t *xoff, int64_t *xoffend, int64_t *xsrc, int64_t *xadj, int64_t *eweight, int64_t *vweight, double *score, int64_t mode)
{
	if (mode == HEAVYEDGE)
	{
		OMP("omp parallel for")
		MTA("mta assert nodep")
		for (int64_t k = 0; k < NV; k++)
		{
			int64_t myStart = xoff[k];
			int64_t myEnd = xoffend[k];
		
			MTA("mta assert nodep")	
			for (int64_t j = myStart; j < myEnd; j++)
			{
				score[j] = (double) eweight[j];
			}
		}
	}
	else if (mode == CNM)
	{
		//int64_t sum_edge_weights = 0.0;

		OMP("omp parallel for")
		MTA("mta assert nodep")
		for (int64_t k = 0; k < NV; k++)
		{
			int64_t sum_edge_weights = 0;
			int64_t myStart = xoff[k];
			int64_t myEnd = xoffend[k];

			MTA("mta assert nodep")
			for (int64_t j = myStart; j < myEnd; j++)
			{
				sum_edge_weights += eweight[j];
			}

			MTA("mta assert nodep")
			for (int64_t j = myStart; j < myEnd; j++)
			{
				int64_t v = xadj[j];
				score[j] = ((double) eweight[j] / (double) sum_edge_weights) - (double) (vweight[k] * vweight[v]) / (double) (4 * sum_edge_weights * sum_edge_weights);
			}
		}
/*
		OMP("omp parallel for")
		MTA("mta assert nodep")
		for (int64_t k = 0; k < NV; k++)
		{
			int64_t myStart = xoff[k];
			int64_t myEnd = xoffend[k];

			MTA("mta assert nodep")
			for (int64_t j = myStart; j < myEnd; j++)
			{
				int64_t v = xadj[j];
				score[j] = (eweight[j] / sum_edge_weights) - (vweight[k] * vweight[v]) / (4 * sum_edge_weights * sum_edge_weights);
			}
		}*/
	}


	return;
}


int64_t MergeSortedLists(int64_t *a, int64_t *aw, int64_t an, int64_t *b, int64_t *bw, int64_t bn, int64_t *c, int64_t *cw, int64_t cn,
 int64_t m)
{
	int64_t a_ptr = 0;
	int64_t b_ptr = 0;
	int64_t c_ptr = 0;

	while (a_ptr < an && b_ptr < bn)
	{
		if (a[a_ptr] <= b[b_ptr])
		{
			if (a[a_ptr] == c[c_ptr-1])
			{
				cw[c_ptr-1] += aw[a_ptr];
			}
			else if (a[a_ptr] != m)
			{
				c[c_ptr] = a[a_ptr];
				cw[c_ptr] = aw[a_ptr];
				c_ptr++;
			}
			a_ptr++;
		}
		else
		{
			if (b[b_ptr] == c[c_ptr-1])
			{
				cw[c_ptr-1] += bw[b_ptr];
			}
			else if (b[b_ptr] != m)
			{
				c[c_ptr] = b[b_ptr];
				cw[c_ptr] = bw[b_ptr];
				c_ptr++;
			}
			b_ptr++;
		}
	}
	
	while (a_ptr < an)
	{
		if (a[a_ptr] == c[c_ptr-1])
		{
			cw[c_ptr-1] += aw[a_ptr];
		}
		else if (a[a_ptr] != m)
		{
			c[c_ptr] = a[a_ptr];
			cw[c_ptr] = aw[a_ptr];
			c_ptr++;
		}
		a_ptr++;
	}

	while (b_ptr < bn)
	{
		if (b[b_ptr] == c[c_ptr-1])
		{
			cw[c_ptr-1] += bw[b_ptr];
		}
		else if (b[b_ptr] != m)
		{
			c[c_ptr] = b[b_ptr];
			cw[c_ptr] = bw[b_ptr];
			c_ptr++;
		}
		b_ptr++;
	}

	return c_ptr;
}


void agglomerative_partitioning (graph *G, double *modularity, int64_t mode)
{
	int64_t NV = G->numVertices;
	int64_t NE = G->numEdges;
	int64_t *membership = G->marks;

	int64_t nCommunities;

	int64_t maxSize = NV;
	int64_t minSize = 1;
	int64_t prevSize = NV + 1;
	int64_t Done = 0;

	size_t bufsize = NE * sizeof(double) +
						NV * sizeof(int64_t) +
						NV * sizeof(int64_t) +
						NV * sizeof(int64_t) +
						NE * sizeof(int64_t) +
						NE * sizeof(int64_t) +
						NE * sizeof(int64_t) +
						NE * sizeof(int64_t) +
						NV * sizeof(int64_t) +
						(NV+2) * sizeof(int64_t);

	void *mem = xmmap (NULL, bufsize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, 0, 0);
	double *score = (double *) mem;
	int64_t *matched = (int64_t *) &score[NE];
	int64_t *pickedV = (int64_t *) &matched[NV];
	int64_t *pickedE = (int64_t *) &pickedV[NV];
	int64_t *temp = (int64_t *) &pickedE[NV];
	int64_t *temp2 = (int64_t *) &temp[NE];
	int64_t *temp3 = (int64_t *) &temp2[NE];
	int64_t *indices = (int64_t *) &temp3[NE];
	int64_t *commSize = (int64_t *) &indices[NE];
	int64_t *temp4 = (int64_t *) &commSize[NV];

/*
	double *score = xmalloc(NE * sizeof(double));
	int64_t *matched = xmalloc(NV * sizeof(int64_t));
	int64_t *matching = xmalloc(NE * sizeof(int64_t));
	int64_t *pickedV = xmalloc(NV * sizeof(int64_t));
	int64_t *pickedE = xmalloc(NV * sizeof(int64_t));
	int64_t *temp = xmalloc(NE * sizeof(int64_t));
	int64_t *temp2 = xmalloc(NE * sizeof(int64_t));
	double *temp3 = xmalloc(NE * sizeof(double));
	int64_t *indices = xmalloc(NE * sizeof(int64_t));
	int64_t *commSize = xmalloc(NV * sizeof (int64_t));
*/

	struct graph_part *G1 = xmalloc(sizeof(struct graph_part));
	struct graph_part *G2 = xmalloc(sizeof(struct graph_part));
	struct graph_part *tmp;
	
	alloc_graph_part(G1, NV, NE);
	alloc_graph_part(G2, NV, NE);


	/* Copy the input graph to the first graph structure */
	OMP("omp parallel for")
	MTA("mta assert nodep")
	for (int64_t k = 0; k < NE; k++)
	{
		G1->xsrc[k] = G->startVertex[k];
		G1->xadj[k] = G->endVertex[k];
	}

	OMP("omp parallel for")
	MTA("mta assert nodep")
	for (int64_t k = 0; k < NV+1; k++)
	{
		G1->xoff[k] = G->edgeStart[k];
		G1->xoffend[k] = G->edgeStart[k+1];
	}

	OMP("omp parallel for")
	MTA("mta assert nodep")
	for (int64_t k = 0; k < NV; k++)
	{
		G1->vweight[k] = 1;   // 1 or zero??
	}

	OMP("omp parallel for")
	MTA("mta assert nodep")
	for (int64_t k = 0; k < NE; k++)
	{
		G1->eweight[k] = 1;
	}

	stats_tic ("agglomerative");
	/* every vertex starts in its own community */
	OMP("omp parallel for")
	MTA("mta assert nodep")
	for (int64_t k = 0; k < NV; k++)
	{
		membership[k] = k;
	}

	int64_t *xoff;
	int64_t *xoffend;
	int64_t *xadj;
	int64_t *xsrc;
	int64_t *vweight;
	int64_t *eweight;

	double time;

	while (1)
	{
		xoff = G1->xoff;
		xoffend = G1->xoffend;
		xadj = G1->xadj;
		xsrc = G1->xsrc;
		vweight = G1->vweight;
		eweight = G1->eweight;


		time = timer();
		/* Calculate edge scores */
		if (mode == 1)
		{
			calcScore(NV, NE, xoff, xoffend, xsrc, xadj, eweight, vweight, score, HEAVYEDGE);
		}
		else if (mode == 2)
		{
			calcScore(NV, NE, xoff, xoffend, xsrc, xadj, eweight, vweight, score, CNM);
		}
		else if (mode == 3)
		{
			calcScore(NV, NE, xoff, xoffend, xsrc, xadj, eweight, vweight, score, MB);
		}
		else if (mode == 4)
		{
			calcScore(NV, NE, xoff, xoffend, xsrc, xadj, eweight, vweight, score, CONDUCTANCE);
		}
#ifdef TIME
		time = timer() - time;  printf("%20.15e, ", time);
#endif

		/* Check for termination */
		time = timer();
		nCommunities = 0;
		OMP("omp parallel for")
		MTA("mta assert nodep")
		for (int64_t k = 0; k < NV; k++)
		{
			commSize[k] = 0;
		}

		MTA("mta assert nodep")
		for (int64_t k = 0; k < NV; k++)
		{
			int64_t m = membership[k];
			if (m == k) nCommunities++;
			if (int64_fetch_add(&commSize[m], 1) >= maxSize) Done = 1;
		}
#if 0 || DEBUG
		printf("nCommunities = %ld\n", nCommunities);
#endif

		if (nCommunities <= minSize) Done = 1;
		if (nCommunities == prevSize) Done = 1;

		if (Done) break;
		prevSize = nCommunities;
#ifdef TIME
		time = timer() - time;  printf("%20.15e, ", time);
#endif


		/* Parallel matching with pointers */
		time = timer();
		parallel_matching(NV, NE, xoff, xoffend, xadj, score, matched, pickedV, pickedE);
#ifdef TIME
		time = timer() - time;  printf("%20.15e, ", time);
#endif

		/* Updating communities & Graph contraction */

#if 0
		/* The next two loops use:
		     16 instructions
			 5 loads
			 6 stores
			 4 branches on the XMT */

		MTA("mta assert nodep")
		for (int64_t k = 0; k < NV; k++)
		{	
			int64_t m = matched[k];
			if (m == -1)     // unmatched
			{
				xoffend[k] = k;
			}
			else if (m > k)   // lower vertex id of matching
			{
				xoffend[k] = k;
				xoffend[m] = k;
				membership[m] = k;
			}
		}

		/* translate vertex numbers in place */
		MTA("mta assert nodep")
		for (int64_t k = 0; k < NE; k++)
		{
			xadj[k] = xoffend[xadj[k]];
			xsrc[k] = xoffend[xsrc[k]];
		}
#endif

		/* The next two loops use:
		     20 instructions
			 5 loads
			 3 stores
			 7 branches on the XMT */

		/* Update community mapping */
		time = timer();
		OMP("omp parallel for")
		MTA("mta assert nodep")
		for (int64_t k = 0; k < NV; k++)
		{	
			int64_t m = matched[k];
			if (m > k)   // lower vertex id of matching
			{
				membership[m] = k;
			}
		}
#ifdef TIME
		time = timer() - time;  printf("%20.15e, ", time);
#endif

		/* Relabel edges */
		time = timer();
		OMP("omp parallel for")
		MTA("mta assert nodep")
		for (int64_t k = 0; k < NE; k++)
		{
			int64_t m = xadj[k];
			int64_t v = matched[m];
			if (v < m && v >= 0) xadj[k] = v;
			m = xsrc[k];
			v = matched[m];
			if (v < m && v >= 0) xsrc[k] = v;
		}

		OMP("omp parallel for")
		MTA("mta assert nodep")
		for (int64_t k = 0; k < NE; k++)
		{
			indices[k] = k;
		}
#ifdef TIME
		time = timer() - time;  printf("%20.15e, ", time);
#endif

		/* translate vertex numbers for vertex weight */
		time = timer();
		OMP("omp parallel for")
		MTA("mta assert nodep")
		for (int64_t k = 0; k < NV; k++)
		{
			int64_t m = matched[k];
			if (m > k)   // lower vertex id of matching
			{
				vweight[k] += vweight[m];
				vweight[m] = 0;
			}
		}

		/* copy vertex weights from G1 to G2 */
		OMP("omp parallel for")
		MTA("mta assert nodep")
		for (int64_t k = 0; k < NV; k++)
		{
			G2->vweight[k] = vweight[k];
		}
#ifdef TIME
		time = timer() - time;  printf("%20.15e, ", time);
#endif

		/* sort each neighbor list with edge weights */
		time = timer();
		global_ptr = xadj;

		OMP("omp parallel for")
		MTA("mta assert nodep")
		MTA("mta assert parallel")
		for (int64_t k = 0; k < NV; k++)
		{
			int64_t myStart = xoff[k];
			int64_t myEnd = xoffend[k];
			int64_t degree = myEnd - myStart;

			if (degree > 1)
			{
				//oddEvenSort(xadj, indices, myStart, myEnd-1);
#ifdef __MTA__
				insertionSort(xadj, indices, myStart, myEnd-1);
#else
				//RobsDumbSort(xadj, indices, myStart, myEnd-1);
				//quickSort(xadj, indices, myStart, myEnd-1);
				qsort(&indices[myStart], degree, sizeof(int64_t), myWeightedCompare);
#endif
			}
		}

#if 0
		int64_t sortError = 0;
		MTA("mta assert nodep")
		for (int64_t k = 0; k < NV; k++)
		{
			int64_t myStart = xoff[k];
			int64_t myEnd = xoffend[k];
			int64_t flag = 0;

			for (int64_t j = myStart+1; j < myEnd; j++)
			{
				if (xadj[indices[j]] < xadj[indices[j-1]])
				{
					int64_fetch_add(&sortError, 1);
//					flag = 1;
				}
			}

/*			if (flag)
			{
				printf("--%ld--\n", k);
				for (int64_t j = myStart; j < myEnd; j++)
				{
					printf("%ld\n", xadj[indices[j]]);
				}
				getchar();
			}*/
		}

		printf("sortError = %ld\n", sortError);
		getchar();
#endif

#ifdef TIME
		time = timer() - time;  printf("%20.15e, ", time);
#endif

		int64_t *G2_xoff = G2->xoff;

		/* initialize G2 offsets */
		time = timer();
		OMP("omp parallel for")
		MTA("mta assert nodep")
		for (int64_t k = 0; k < NV+1; k++)
		{
			G2_xoff[k] = 0;
		}

		/* find G2 max vertex degrees */
		MTA("mta assert nodep")
		for (int64_t k = 0; k < NV; k++)
		{
			int64_t deg = xoffend[k] - xoff[k];
			int64_t v;
			if (matched[k] == -1) v = k;
			else if (matched[k] > k) v = k;
			else v = matched[k];

			int64_fetch_add(&G2_xoff[v+1], deg);
		}
#ifdef TIME
		time = timer() - time;  printf("%20.15e, ", time);
#endif


		/* calculate G2 offsets */
		time = timer();
		for (int64_t k = 1; k < NV+1; k++)
		{
			G2_xoff[k] += G2_xoff[k-1];
		}

		OMP("omp parallel for")
		MTA("mta assert nodep")
		for (int64_t k = 0; k < NV+1; k++)
		{
			G2->xoffend[k] = G2_xoff[k];
		}
#ifdef TIME
		time = timer() - time;  printf("%20.15e, ", time);
#endif

		/* For unmatched vertices, copy edges into G2, aggregating duplicates */
		/* For matched vertices, combine and merge edges, copy into G2, aggregating
		 * duplicates and removing self loops.  */
		time = timer();
		OMP("omp parallel for")
		MTA("mta assert nodep")
		for (int64_t k = 0; k < NV; k++)
		{
			int64_t v = matched[k];
			if (v == -1)
			{
				int64_t myStart = xoff[k];
				int64_t myEnd = xoffend[k];
				int64_t myHead = G2_xoff[k];

				//MTA("mta assert nodep")
				for (int64_t j = myStart; j < myEnd-1; j++)
				{
					if (xadj[indices[j]] == k) continue;   // skip self-edges
					if (xadj[indices[j]] != xadj[indices[j+1]])
					{
						G2->xsrc[myHead] = k;
						G2->xadj[myHead] = xadj[indices[j]];
						G2->eweight[myHead] = eweight[indices[j]];
						myHead++;
					}
					else
					{
						eweight[indices[j+1]] += eweight[indices[j]];
					}
				}

				if (myStart != myEnd && xadj[indices[myEnd-1]] != k)
				{
					G2->xsrc[myHead] = k;
					G2->xadj[myHead] = xadj[indices[myEnd-1]];
					G2->eweight[myHead] = eweight[indices[myEnd-1]];
					myHead++;
				}
				
				G2->xoffend[k] = myHead;
			}
		}
#ifdef TIME
		time = timer() - time;  printf("%20.15e, ", time);
#endif

		time = timer();
		OMP("omp parallel for")
		MTA("mta assert nodep")
		for (int64_t k = 0; k < NV; k++)
		{
			int64_t v = matched[k];
			if (v > k)
			{
				int64_t k_start = xoff[k];
				int64_t k_end = xoffend[k];
				int64_t v_start = xoff[v];
				int64_t v_end = xoffend[v];

				int64_t a_ptr = k_start;
				int64_t b_ptr = v_start;
				int64_t c_ptr = G2_xoff[k];

				while (a_ptr < k_end && b_ptr < v_end)
				{
					if (xadj[indices[a_ptr]] <= xadj[indices[b_ptr]])
					{
						if (xadj[indices[a_ptr]] == G2->xadj[c_ptr-1])
						{
							G2->eweight[c_ptr-1] += eweight[indices[a_ptr]];
						}
						else if (xadj[indices[a_ptr]] != k)
						{
							G2->xsrc[c_ptr] = k;
							G2->xadj[c_ptr] = xadj[indices[a_ptr]];
							G2->eweight[c_ptr] = eweight[indices[a_ptr]];
							c_ptr++;
						}
						a_ptr++;
					}
					else
					{
						if (xadj[indices[b_ptr]] == G2->xadj[c_ptr-1])
						{
							G2->eweight[c_ptr-1] += eweight[indices[b_ptr]];
						}
						else if (xadj[indices[b_ptr]] != k)
						{
							G2->xsrc[c_ptr] = k;
							G2->xadj[c_ptr] = xadj[indices[b_ptr]];
							G2->eweight[c_ptr] = eweight[indices[b_ptr]];
							c_ptr++;
						}
						b_ptr++;
					}
				}
				
				while (a_ptr < k_end)
				{
					if (xadj[indices[a_ptr]] == G2->xadj[c_ptr-1])
					{
						G2->eweight[c_ptr-1] += eweight[indices[a_ptr]];
					}
					else if (xadj[indices[a_ptr]] != k)
					{
						G2->xsrc[c_ptr] = k;
						G2->xadj[c_ptr] = xadj[indices[a_ptr]];
						G2->eweight[c_ptr] = eweight[indices[a_ptr]];
						c_ptr++;
					}
					a_ptr++;
				}

				while (b_ptr < v_end)
				{
					if (xadj[indices[b_ptr]] == G2->xadj[c_ptr-1])
					{
						G2->eweight[c_ptr-1] += eweight[indices[b_ptr]];
					}
					else if (xadj[indices[b_ptr]] != k)
					{
						G2->xsrc[c_ptr] = k;
						G2->xadj[c_ptr] = xadj[indices[b_ptr]];
						G2->eweight[c_ptr] = eweight[indices[b_ptr]];
						c_ptr++;
					}
					b_ptr++;
				}

				G2->xoffend[k] = c_ptr;
			}
		}
#ifdef TIME
		time = timer() - time;  printf("%20.15e\n", time);
#endif

#ifdef DEBUG
		/* check the new graph */
		int64_t errors = 0;
		for (int64_t k = 0; k < G2->NV; k++)
		{
			int64_t myStart = G2->xoff[k];
			int64_t myEnd = G2->xoffend[k];

			for (int64_t j = myStart; j < myEnd; j++)
			{
				if (G2->xsrc[j] != k) errors++;
				if (G2->xadj[j] < 0) errors++;
				if (G2->xadj[j] >= NV) errors++;
			}
		}
		printf("errors: %ld\n", errors);
		errors = 0;
		for (int64_t k = 0; k < G2->NV+1; k++)
		{
			if (G2->xoff[k] < 0) errors++;
			if (G2->xoff[k] > G->numEdges) errors++;
			if (G2->xoffend[k] < 0) errors++;
			if (G2->xoffend[k] > G->numEdges) errors++;
		}
		printf("errors: %ld\n", errors);

		printf("%ld %ld\n", G2->NV, G2->NE);
		printf("%ld %ld %ld %ld %ld %ld\n", G2->xoff[5], G2->xoffend[5], G2->xsrc[5], G2->xadj[5], G2->eweight[5], G2->vweight[5]);
		getchar();
#endif		

		/* Swap pointers G1 and G2 */
		tmp = G1;
		G1 = G2;
		G2 = tmp;

		/* and repeat... */

	}
	stats_toc ();
}
