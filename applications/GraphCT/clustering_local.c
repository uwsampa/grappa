#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include "defs.h"
#include "globals.h"
#ifdef __MTA__
#include <sys/mta_task.h>
#include <machine/runtime.h>
#else
#include "compat/xmt-ops.h"
#endif
#include <math.h>
#include <inttypes.h>
#include "stinger-atomics.h"


MTA("mta inline")
int64_t count_intersect(int64_t * start, int64_t * eV, int64_t * start2, int64_t * eV2, int64_t u, int64_t v)
{
	int64_t begin1 = start[u];
	int64_t end1 = start[u+1];
	int64_t begin2 = start2[v];
	int64_t end2 = start2[v+1];
	int64_t count = 0;

	while(begin1 < end1 && begin2 < end2)
	{
		
		if(eV[begin1] == eV2[begin2]) 
		{
			count++;
			begin1++;
			begin2++;
		}
		else if(eV[begin1] < eV2[begin2]) begin1++;
		else begin2++;
	}
	
	return count;
}

int compare(const void * a, const void * b)
{
	return (int) (*(int64_t*)a - *(int64_t*)b);
}

void SortStart2(int64_t NV, int64_t NE, int64_t * sv1, int64_t * ev1, int64_t * ev2, int64_t * start)
{
	int64_t i;

	OMP("omp parallel for")
	for (i = 0; i < NV + 2; i++) start[i] = 0;

	start += 2;

/* Histogram key values */
	OMP("omp parallel for")
	MTA("mta assert no alias *start *sv1")
//	for (i = 0; i < NE; i++) start[sv1[i]] ++;
	for (i = 0; i < NE; i++) stinger_int_fetch_add(&start[sv1[i]], 1);

/* Compute start index of each bucket */
	for (i = 1; i < NV; i++) start[i] += start[i-1];

	start --;

/* Move edges into its bucket's segment */
	OMP("omp parallel for")
	MTA("mta assert no dependence")
	for (i = 0; i < NE; i++) {
//		int64_t index = start[sv1[i]] ++;
		int64_t index = stinger_int_fetch_add(&start[sv1[i]], 1);
		ev2[index] = ev1[i];
	}
}

void SortGraphEdgeLists(graph *G)
{
	int64_t i;
	int64_t NV = G->numVertices;
	int64_t * start = G->edgeStart;
	int64_t * eV = G->endVertex;

	OMP("omp parallel for")
	MTA("mta assert parallel")
	MTA("mta dynamic schedule")
	for (i = 0; i < NV; i++)
	{
		const int64_t myStart = start[i];
		const int64_t deg = start[i+1] - myStart;

		if (deg > 1)
		{
			qsort(&eV[myStart], deg, sizeof(int64_t), compare);
		}
	}
}


double * calculateClusteringLocal(graph* G)
{
	int64_t i;
	int64_t NV = G->numVertices;
	int64_t * start = G->edgeStart;
	int64_t * eV = G->endVertex;

	static int64_t * restrict ntri;
	static double * restrict local_cc;
	
	ntri = xcalloc (NV, sizeof(*ntri));
	local_cc = xcalloc(NV, sizeof(*local_cc));

	count_all_triangles (NV, start, eV, ntri);

	OMP("omp parallel for")
	MTA("mta assert nodep")
	MTA("mta use 100 streams")
	for (i = 0; i < NV; i++)
	{
		const int64_t deg = start[i+1] - start[i];
		const int64_t d = deg * (deg-1);
		local_cc[i] = (d ? ntri[i] / (double) d : 0.0);
	}

	return local_cc;
}


MTA("mta expect parallel context")
MTA("mta inline")
size_t
count_intersections (const int64_t ai, const size_t alen, int64_t * a,
		     const int64_t bi, const size_t blen, int64_t * b)
{
	size_t ka = 0, kb = 0;
    size_t out = 0;

    if (!alen || !blen || a[alen-1] < b[0] || b[blen-1] < a[0])
		return 0;

    while (1)
	{
		if (ka >= alen || kb >= blen) break;

		int64_t va = a[ka];
		int64_t vb = b[kb];

		/* Skip self-edges. */
		if (UNLIKELY(va == ai))
		{
			++ka;
			if (ka >= alen) break;
			va = a[ka];
		}
		if (UNLIKELY(vb == bi))
		{
			++kb;
			if (kb >= blen) break;
			vb = b[kb];
		}

		if (va == vb)
		{
			++ka;
			++kb;
			++out;
		}
		else if (va < vb)
		{
			/* Advance ka */
			++ka;
			while (ka < alen && a[ka] < vb) ++ka;
		} else
		{
			/* Advance kb */
			++kb;
			while (kb < blen && va > b[kb]) ++kb;
		}
	}

    return out;
}


MTA("mta expect parallel context")
MTA("mta inline")
int64_t
count_triangles (const size_t nv, int64_t * off,
		 int64_t * ind, int64_t i)
/* Assume sorted index lists. */
{
    int64_t out = 0;
    const int64_t * restrict iind = &ind[off[i]];
    const size_t ideg = off[i+1] - off[i];

    MTA("mta assert nodep")
	MTA("mta assert noalias")
    for (size_t k = 0; k < ideg; ++k) {
		const int64_t j = iind[k];
		if (j != i) {
			const int64_t * restrict jind = &ind[off[j]];
			const size_t jdeg = off[j+1] - off[j];
			const size_t shared = count_intersections (i, ideg, iind, j, jdeg, jind);
			out += (int64_t) shared;
		}
    }

    return out;
}


MTA("mta expect serial context")
void
count_all_triangles (const size_t nv, int64_t * off,
		     int64_t * ind, int64_t * restrict ntri)
{
	const size_t N = nv;
	OMP("omp for schedule(dynamic,128)")
	MTA("mta loop future")
	for (size_t i = 0; i < N; ++i)
	{
		ntri[i] = count_triangles (nv, off, ind, i);
	}
}

