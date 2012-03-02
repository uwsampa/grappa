#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include "defs.h"
#include "globals.h"
#ifdef __MTA__
#include <sys/mta_task.h>
#include <machine/runtime.h>
#define int64_fetch_add int_fetch_add
#else
#include "compat/xmt-ops.h"
#endif
#include <math.h>
#include <sys/mman.h>
#include "stinger-atomics.h"

/* Set this variable to 1 to time each iteration */
#define TIMEBCPART 0


static int64_t W_recurse (int64_t, int64_t, int64_t, int64_t *, int64_t);

MTA("mta inline")
static int64_t intpow(int64_t base, int64_t exp)
{
	int64_t tmp;
	switch(exp)
	{
		case 0: return 1;
		case 1: return base;
		case 2: return base*base;
		case 3: return base*base*base;
		case 4: tmp = base*base;
				  return tmp*tmp;
		case 5: tmp = base*base;
				  return tmp*tmp*base;
		default: return (int64_t) pow(base, exp);
	}
}

static void kcent_core(graph * G, double *BC, int64_t K, int64_t s, int64_t * Q, int64_t * dist, int64_t * sigma, int64_t * marks, int64_t * QHead, int64_t * child, int64_t * child_count)
{
	int64_t NE      = G->numEdges;
	int64_t *eV     = G->endVertex;
	int64_t NV      = G->numVertices;
	int64_t *start  = G->edgeStart;

	int64_t d_phase;
	int64_t i, j, k, p, n;
	int64_t nQ, Qnext, Qstart, Qend;
	int64_t kprime;

	/* Reuse the dist memory in the accumulation phase */
	double *delta = (double *) dist;

	OMP("omp parallel for")
	MTA("mta assert nodep")
	for (j = 0; j < NV; j++)
	{
		marks[j] = sigma[NV*(K+1) + j] = 0;
	}

/*
MTA("mta assert nodep")
MTA("mta assert noalias *dist *sigma *child_count")
MTA("mta assert parallel")
	for (j = 0; j < (K+1); j++) {
MTA("mta assert nodep")
MTA("mta assert no alias *dist *sigma *child_count")
MTA("mta assert parallel")
		int64_t childset = j*NE;
		for(k = j*NV; k<(j+1)*NV; k++)
		{
			dist[k] = -1; 
			sigma[k] = 0; 
			child_count[k] = childset;
		}
	}
	*/

	OMP("omp parallel for")
	MTA("mta assert nodep")
	for (j = 0; j < (K+1)*NV; j++) {
			dist[j] = -1; 
			sigma[j] = child_count[j] = 0;
	}

#if TIMEBCPART      
		timep1 = timer() - timep1;
		fprintf(stderr, "Src: %d, initialization time: %9.6lf sec.\n", s, timep1);
		timep1 = timer();
#endif


		/* Push node i onto Q and set bounds for first Q sublist */
		Q[0]     = s;
		Qnext    = 1;

		nQ       = 1;
		QHead[0] = 0;
		QHead[1] = 1;

		dist[s]  = 0;
		marks[s] = 1;
		sigma[s] = 1;
		sigma[(K+1)*NV + s] = 1;
PushOnStack:    /* Push nodes onto Q */

		/* Execute the nested loop for each node v on the Q AND
			for each neighbor w of v whose edge weight is not divisible by 8
		 */
		d_phase = nQ;
		Qstart = QHead[nQ-1];
		Qend = QHead[nQ];

		MTA("mta assert no dependence")
		MTA("mta block dynamic schedule")
		MTA("mta use 60 streams")
		for (j = Qstart; j < Qend; j++) {
			int64_t v = Q[j];
			int64_t sigmav  = sigma[v];    
			int64_t myStart = start[v];     
			int64_t myEnd   = start[v+1];
			int64_t min;
			min = K;
			if (1 < K) min = 1;

			//MTA("mta assert no dependence")
			//MTA("mta block dynamic schedule")
			//MTA("mta assert no alias *weight *eV *dist *marks *sigma *child_count *child")
			for (k = myStart; k < myEnd; k++) {
				int64_t d, w, l;
				w = eV[k];
				d = dist[w]; 
				/* If node has not been visited, set distance and push on Q (but only once) */

				if (d < 0) {
					if (stinger_int_fetch_add(&marks[w], 1) == 0) {
						dist[w] = d_phase; Q[stinger_int_fetch_add(&Qnext, 1)] = w;

					}
					stinger_int_fetch_add(&sigma[w], sigmav);
					stinger_int_fetch_add(&sigma[(K+1)*NV + w], sigmav);
					l = myStart + stinger_int_fetch_add(&child_count[v], 1);
					child[l] = w;
				}
				else if(d == d_phase)
				{
					stinger_int_fetch_add(&sigma[w], sigmav);
					stinger_int_fetch_add(&sigma[(K+1)*NV + w], sigmav);
					l = myStart + stinger_int_fetch_add(&child_count[v], 1);
					child[l] = w;
				}
				else if(K > 0 && d == dist[v])
				{
					stinger_int_fetch_add(&sigma[NV + w], sigmav);
					stinger_int_fetch_add(&sigma[(K+1)*NV + w], sigmav);
					l = myStart + stinger_int_fetch_add(&child_count[NV + v], 1);
					child[l + NE] = w;
				}
			}
		}

		/* If new nodes pushed onto Q */
		if (Qnext != QHead[nQ]) 
		{
			nQ++;
			QHead[nQ] = Qnext; 
			goto PushOnStack;
		}


		for (i = 1; i <= K; i++)
		{
			for (p = 0; p < nQ; p++) 
			{
				MTA("mta assert nodep")
				MTA("mta assert no alias *sigma *Q *child *start *QHead")
				MTA("mta use 60 streams")
				for (n = QHead[p]; n < QHead[p+1]; n++) 
				{
					int64_t v = Q[n];
					int64_t myStart = start[v];
					int64_t myEnd = myStart + child_count[v];

					for (k = myStart; k < myEnd; k++)
					{
						int64_t w = child[k];
						stinger_int_fetch_add(&sigma[(i*NV) + w], sigma[i*NV + v]);
						stinger_int_fetch_add(&sigma[(K+1)*NV + w], sigma[i*NV + v]);
					}

					if (i < K)
					{
						for (j = 1; j <= i + 1; j++)
						{
							int64_t myEnd = myStart + child_count[j*NV + v];

							for (k = myStart; k < myEnd; k++)
							{
								int64_t w = child[k + j*NE];
								stinger_int_fetch_add(&sigma[((i+1)*NV) + w], sigma[(i+1-j)*NV + v]);
								stinger_int_fetch_add(&sigma[(K+1)*NV + w], sigma[(i+1-j)*NV + v]);
							}
						}
					}
				}
			}
		}

		nQ--;

		OMP("omp parallel for")
		for (j=0; j<(K+1)*NV; j++) delta[j] = 0.0;

		for (kprime = 0; kprime <= K; kprime++)
		{
			for (p = nQ; p > 1; p--)
			{
				Qstart = QHead[p-1];
				Qend = QHead[p];

				MTA("mta assert nodep")
				MTA("mta block dynamic schedule")
				MTA("mta assert no alias *sigma *Q *BC *delta *child *start *QHead")
				MTA("mta use 60 streams")
				for (n = Qstart; n < Qend; n++)
				{
					int64_t v = Q[n];
					int64_t d, i, w, e, myStart, myEnd, diff, iter;
					int64_t multi;
					myStart = start[v];
					myEnd = myStart + child_count[kprime*NV + v];
					
					for (k = myStart; k < myEnd; k++)
					{
						w = child[kprime*NE + k];
						double tmp = sigma[v]*(1.0/sigma[(K+1)*NV + w] + delta[w]/sigma[w]);
						delta[kprime*NV + v] += tmp; 
					}

					if (kprime > 0)
					{
						i=1;
						iter = kprime - 1;
						myEnd = myStart + child_count[iter*NV + v];

						for(k = myStart; k < myEnd; k++)
						{
							w = child[iter*NE + k];
							double tmp;
							tmp = sigma[NV + v]/(double) sigma[(K+1)*NV + w];
							if (delta[w] && sigma[w])
								tmp += delta[w]/sigma[w]*(sigma[NV + v] - sigma[NV + w]*sigma[v]/sigma[w]);
							delta[kprime*NV + v] += tmp;
							tmp= sigma[v]*delta[NV + w]/sigma[w];
							delta[kprime*NV + v] += tmp; 
						}
						
						if(kprime > 1)
						{
							iter = kprime - 2;
							myEnd = myStart + child_count[iter*NV + v];

							for(k = myStart; k < myEnd; k++)
							{
								w = child[k + iter*NE];
								delta[kprime*NV + v] += (double) (sigma[v] * (delta[2*NV + w]))/(double) sigma[w];
								delta[kprime*NV + v] += (double) (sigma[NV + v] - sigma[NV + w]*sigma[v]/sigma[w])*delta[NV + w]/(double) (sigma[w]);
								delta[kprime*NV + v] += (double) (sigma[2*NV + v] - sigma[NV + w]*sigma[NV + v]/(double) sigma[w] + (sigma[NV + w]*sigma[NV + w] - sigma[w]*sigma[2*NV + w])*sigma[v]/(sigma[w]*sigma[w]))*delta[w]/(double) sigma[w];
								delta[kprime*NV + v] += (double) sigma[2*NV + v]/(double) sigma[(K+1)*NV + w];
							}

							for (d = 0; d <= kprime-3; d++)
							{
								k = myStart;
								myEnd = k + child_count[d*NV + v];
								diff = kprime - d + 1;
								iter = (myEnd - k)*diff;
								e=0;
								multi=1;

								for (i = 0; i < iter; i++) 
								{
									w = child[k + d*NE];
									multi *= sigma[w];
									int64_t sum = 0;

									for (j = 0; j <= e; j++)
									{
										sum += W_recurse(e-j, e, w, sigma, NV)*sigma[NV*j + v];
									}
									delta[kprime*NV + v] += (double) (sum * (delta[(kprime - d - e)*NV + w])) / (double) multi;

									e++;
									if(e == diff) 
									{	
										e = 0;
										multi = 1;
										double tmp = (double) sigma[(kprime-d)*NV + v] / (double) sigma[(K+1)*NV + w];
										delta[kprime*NV + v] += tmp; 
										k++;
									}
								}
							}
						}
					}
					//if(isnan(delta[kprime*NV + v])) printf("NAN at v = %d, kprime = %d\n", v, kprime);
					MTA("mta update")
					BC[v] += delta[kprime*NV + v];
				}
			}
		}



}

double kcentrality(graph *G, double *BC, int64_t Vs, int64_t K)
{
	double timeBC, timep1, timep2, timer();

	int64_t numSkipped = 0;
	int64_t NV      = G->numVertices;
	int64_t NE      = G->numEdges;
	int64_t *start  = G->edgeStart;
	int64_t *explored = xmalloc(sizeof(int64_t)*NV);
	int64_t j;

	OMP("omp parallel for")
	for (j = 0; j < NV; j++) 
	{
		BC[j] = 0.0;
		explored[j] = j;
	}

	double *rn;
	rn = xmalloc(Vs * sizeof(double));
	prand(Vs, rn);
  
	MTA("mta assert nodep")
	for (j = 0; j < Vs; j++)
	{
		int64_t swap = (int64_t) (rn[j]*(NV - j)) + j;
		int64_t tmp = explored[swap];
		explored[swap] = explored[j];
		explored[j] = tmp;
	}


	int64_t x;
	/* Use |Vs| nodes to compute centrality values */
	int64_t k = 0;

#if defined(__MTA__)
#define INC 16
#else
#define INC 1
#endif

	// Here I created a size variable for the xmmap() call
	int64_t buffsz = INC*NV * sizeof(int64_t)       +
			INC*(K+1) * NV * sizeof(double) +
			INC*(K+2) * NV * sizeof(int64_t)    +
			INC*(NV + 2) * sizeof(int64_t)      +
			INC*10 * SCALE * sizeof(int64_t)    +
			INC*(K+1) * NE * sizeof(int64_t)    +
			INC*(K+1) * NV * sizeof(int64_t);

	// call xmmap()
	int64_t *buff = xmmap(NULL,buffsz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, 0,0);

	/* Allocate memory for data structures */
	int64_t *Qbig = buff;

	// Here I simply offset each pointer into the xmmap()'ed space at the
	// appropriate location
	/* dist's memory is used for both ints and doubles (as delta). */
	int64_t *distbig = (int64_t *) &buff[INC*NV];
	int64_t *sigmabig = (int64_t *) &distbig[INC*(K+1) * NV];
	int64_t *marksbig = (int64_t *) &sigmabig[INC*(K+2) * NV];
	int64_t *QHeadbig = (int64_t *) &marksbig[INC*(NV + 2)];
	int64_t *childbig = (int64_t *) &QHeadbig[INC*10 * SCALE];
	int64_t *child_countbig = (int64_t *) &childbig[INC*(K+1)*NE];

	timeBC = timer();
	
	MTA("mta assert parallel")
	MTA("mta loop future")
	for(x = 0; x < INC; x++)
	{
		int64_t *Q = Qbig + x*NV;
		int64_t *dist = distbig + x*(K+1)*NV;
		int64_t *sigma = sigmabig + x*(K+2)*NV;
		int64_t *marks = marksbig + x*(NV+2);
		int64_t *QHead = QHeadbig + x*10*SCALE;
		int64_t *child = childbig + x*(K+1)*NE;
		int64_t *child_count = child_countbig + x*(K+1)*NV;

		for (int64_t claimedk = stinger_int_fetch_add (&k, 1); claimedk < Vs;
		     claimedk = stinger_int_fetch_add (&k, 1))
		{
			int64_t s = explored[claimedk];
			if (start[s+1] == start[s])
			{
				numSkipped++;
			}
			else
			{
#if TIMEBCPART      
				timep1 = timer();
				timep2 = timer();
#endif

				kcent_core(G, BC, K, s, Q, dist, sigma, marks, QHead, child, child_count);

#if TIMEBCPART 
				timep1 = timer() - timep1;
				fprintf(stderr, "Accumulation time: %9.6lf sec.\n", timep1);
				timep2 = timer() - timep2;
				fprintf(stderr, "Src: %d, total time: %9.6lf sec.\n", s, timep2);
#endif
			}
		}
	}

	timeBC = timer() - timeBC;
	munmap(buff,buffsz);
	free(explored);
	free(rn);

	return timeBC;
}

static int64_t W_realrecurse(int64_t k, int64_t d, int64_t w, int64_t * sigma, int64_t NV)
{
	int64_t sum = 0;
	if(k==0) return intpow(sigma[w], d);
	else
	{
		int64_t i;
MTA("mta assert parallel")
		for(i=1; i<=k; i++)
		{
			sum -= sigma[i*NV + w]*W_recurse(k-i, d-1, w, sigma, NV);
		}
	}
	return sum;
}

MTA("mta inline")
int64_t W_recurse(int64_t k, int64_t d, int64_t w, int64_t * sigma, int64_t NV)
{
	switch(k)
	{
		case 0: 
			switch(d)
			{
				case 0: return 1;
				case 1: return sigma[w];
				case 2: return sigma[w]*sigma[w];
				default: return intpow(sigma[w], d);
			}
		case 1:
			switch(d)
			{
				case 1: return -sigma[NV + w];
				case 2: return -sigma[w]*sigma[NV + w];
				default: return -intpow(sigma[w], d-1)*sigma[NV + w];
			}
		case 2:
			switch(d)
			{
				case 2: return sigma[NV + w]*sigma[NV + w] - sigma[w]*sigma[2*NV + w];
				default: return intpow(sigma[w], d-2)*(sigma[NV + w]*sigma[NV + w] - sigma[w]*sigma[2*NV + 2]);
			}
		default:
			return W_realrecurse(k, d, w, sigma, NV);
	}
}
