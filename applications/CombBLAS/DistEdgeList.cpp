/****************************************************************/
/* Parallel Combinatorial BLAS Library (for Graph Computations) */
/* version 1.2 -------------------------------------------------*/
/* date: 10/06/2011 --------------------------------------------*/
/* authors: Aydin Buluc (abuluc@lbl.gov), Adam Lugowski --------*/
/****************************************************************/
/*
Copyright (c) 2011, Aydin Buluc

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <mpi.h>

#include "SpParMat.h"
#include "ParFriends.h"
#include "Operations.h"

#ifndef GRAPH_GENERATOR_SEQ
#define GRAPH_GENERATOR_SEQ
#endif

#include "graph500-1.2/generator/graph_generator.h"
#include "graph500-1.2/generator/utils.h"
#include "RefGen21.h"

#include <fstream>
#include <algorithm>
using namespace std;

template <typename IT>
DistEdgeList<IT>::DistEdgeList(): edges(NULL), pedges(NULL), nedges(0), globalV(0)
{
	commGrid.reset(new CommGrid(MPI_COMM_WORLD, 0, 0));
}

template <typename IT>
DistEdgeList<IT>::DistEdgeList(const char * filename, IT globaln, IT globalm): edges(NULL), pedges(NULL), globalV(globaln)
{
	commGrid.reset(new CommGrid(MPI_COMM_WORLD, 0, 0));

	int nprocs = commGrid->GetSize();
	int rank = commGrid->GetRank();
	nedges = (rank == nprocs-1)? (globalm - rank * (globalm / nprocs)) : (globalm / nprocs);

	FILE * infp = fopen(filename, "rb");
	assert(infp != NULL);
	IT read_offset_start, read_offset_end;
	read_offset_start = rank * 8 * (globalm / nprocs);
	read_offset_end = (rank+1) * 8 * (globalm / nprocs);
	if (rank == nprocs - 1)
   		read_offset_end = 8*globalm;


	ofstream oput;
	commGrid->OpenDebugFile("BinRead", oput);
	if(infp != NULL)
	{	
		oput << "File exists" << endl;
		oput << "Trying to read " << nedges << " edges out of " << globalm << endl;
	}
	else
	{
		oput << "File does not exist" << endl;
	}
	
	/* gen_edges is an array of unsigned ints of size 2*nedges */
	uint32_t * gen_edges = new uint32_t[2*nedges];
	fseek(infp, read_offset_start, SEEK_SET);
	fread(gen_edges, 2*nedges, sizeof(uint32_t), infp);
	SetMemSize(nedges);
	oput << "Freads done " << endl;
	for(IT i=0; i< 2*nedges; ++i)
		edges[i] = (IT) gen_edges[i];
	oput << "Puts done " << endl;
	delete [] gen_edges;
	oput.close();
	
}

template <typename IT>
void DistEdgeList<IT>::Dump64bit(string filename)
{
	int rank,nprocs;
	MPI_Comm World = commGrid->GetWorld();
	MPI_Comm_rank(World, &rank);
	MPI_Comm_size(World, &nprocs);
	MPI_File thefile;
	MPI_File_open(World, filename.c_str(), MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &thefile);    

	IT * prelens = new IT[nprocs];
	prelens[rank] = 2*nedges;
	MPI_Allgather(MPI_IN_PLACE, 0, MPIType<IT>(), prelens, 1, MPIType<IT>(), commGrid->GetWorld());
	IT lengthuntil = accumulate(prelens, prelens+rank, 0);

	// The disp displacement argument specifies the position 
	// (absolute offset in bytes from the beginning of the file) 
    	MPI_File_set_view(thefile, int64_t(lengthuntil * sizeof(IT)), MPIType<IT>(), MPIType<IT>(), "native", MPI_INFO_NULL);
	MPI_File_write(thefile, edges, prelens[rank], MPIType<IT>(), NULL);
	MPI_File_close(&thefile);
	delete [] prelens;
}	

template <typename IT>
void DistEdgeList<IT>::Dump32bit(string filename)
{
	int rank, nprocs;
	MPI_Comm World = commGrid->GetWorld();
	MPI_Comm_rank(World, &rank);
	MPI_Comm_size(World, &nprocs);
	MPI_File thefile;
	MPI_File_open(World, filename.c_str(), MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &thefile);    

	IT * prelens = new IT[nprocs];
	prelens[rank] = 2*nedges;
	MPI_Allgather(MPI_IN_PLACE, 0, MPIType<IT>(), prelens, 1, MPIType<IT>(), commGrid->GetWorld());
	IT lengthuntil = accumulate(prelens, prelens+rank, static_cast<IT>(0));

	// The disp displacement argument specifies the position 
	// (absolute offset in bytes from the beginning of the file) 
    	MPI_File_set_view(thefile, int64_t(lengthuntil * sizeof(uint32_t)), MPI_UNSIGNED, MPI_UNSIGNED, "native", MPI_INFO_NULL);
	uint32_t * gen_edges = new uint32_t[prelens[rank]];
	for(IT i=0; i< prelens[rank]; ++i)
		gen_edges[i] = (uint32_t) edges[i];

	MPI_File_write(thefile, gen_edges, prelens[rank], MPI_UNSIGNED, NULL);
	MPI_File_close(&thefile);

	delete [] prelens;
	delete [] gen_edges;
}	

template <typename IT>
DistEdgeList<IT>::~DistEdgeList()
{
	if(edges) delete [] edges;
	if(pedges) delete [] pedges;
}

//! Allocates enough space
template <typename IT>
void DistEdgeList<IT>::SetMemSize(IT ne)
{
	if (edges)
	{
		delete [] edges;
		edges = NULL;
	}
	
	memedges = ne;
	edges = 0;
	
	if (memedges > 0)
		edges = new IT[2*memedges];
}

/** Removes all edges that begin with a -1. 
 * Walks back from the end to tighten the nedges counter, then walks forward and replaces any edge
 * with a -1 source with the last edge.
 */
template <typename IT>
void DistEdgeList<IT>::CleanupEmpties()
{

	// find out how many edges there actually are
	while (nedges > 0 && edges[2*(nedges-1) + 0] == -1)
	{
		nedges--;
	}
	
	// remove marked multiplicities or self-loops
	for (IT i = 0; i < (nedges-1); i++)
	{
		if (edges[2*i + 0] == -1)
		{
			// the graph500 generator marked this edge as a self-loop or a multiple edge.
			// swap it with the last edge
			edges[2*i + 0] = edges[2*(nedges-1) + 0];
			edges[2*i + 1] = edges[2*(nedges-1) + 1];
			edges[2*(nedges-1) + 0] = -1; // mark this spot as unused

			while (nedges > 0 && edges[2*(nedges-1) + 0] == -1)	// the swapped edge might be -1 too
				nedges--;
		}
	}
}


/**
 * Note that GenGraph500Data will return global vertex numbers (from 1... N). The ith edge can be
 * accessed with edges[2*i] and edges[2*i+1]. There will be duplicates and the data won't be sorted.
 * Generates an edge list consisting of an RMAT matrix suitable for the Graph500 benchmark.
*/
template <typename IT>
void DistEdgeList<IT>::GenGraph500Data(double initiator[4], int log_numverts, int edgefactor, bool scramble, bool packed)
{
	if(packed && (!scramble))
	{
		SpParHelper::Print("WARNING: Packed version does always generate scrambled vertex identifiers\n");
	}

	globalV = ((int64_t)1)<< log_numverts;
	int64_t globaledges = globalV * static_cast<int64_t>(edgefactor);

	if(packed)
	{
		RefGen21::make_graph (log_numverts, globaledges, &nedges, (packed_edge**)(&pedges));
	}
	else
	{
		// The generations use different seeds on different processors, generating independent 
		// local RMAT matrices all having vertex ranges [0,...,globalmax-1]
		// Spread the two 64-bit numbers into five nonzero values in the correct range
		uint_fast32_t seed[5];

		uint64_t size = (uint64_t) commGrid->GetSize();
		uint64_t rank = (uint64_t) commGrid->GetRank();
	#ifdef DETERMINISTIC
		uint64_t seed2 = 2;
	#else
		uint64_t seed2 = time(NULL);
	#endif
		make_mrg_seed(rank, seed2, seed);	// we give rank as the first seed, so it is different on processors

		// a single pair of [val0,val1] for all the computation, global across all processors
		uint64_t val0, val1; /* Values for scrambling */
		if(scramble)
		{
			if(rank == 0)		
				RefGen21::MakeScrambleValues(val0, val1, seed);	// ignore the return value

			MPI_Bcast(&val0, 1, MPIType<uint64_t>(),0, commGrid->GetWorld());
 			MPI_Bcast(&val1, 1, MPIType<uint64_t>(),0, commGrid->GetWorld());
		}

		nedges = globaledges/size;
		SetMemSize(nedges);	
		// clear the source vertex by setting it to -1
		for (IT i = 0; i < nedges; i++)
			edges[2*i+0] = -1;
	
		generate_kronecker(0, 1, seed, log_numverts, nedges, initiator, edges);
		if(scramble)
		{
			for(IT i=0; i < nedges; ++i)
			{
				edges[2*i+0] = RefGen21::scramble(edges[2*i+0], log_numverts, val0, val1);
				edges[2*i+1] = RefGen21::scramble(edges[2*i+1], log_numverts, val0, val1);
			}
		}
	}
}


/**
 * Randomly permutes the distributed edge list.
 * Once we call Viral's psort on this vector, everything will go to the right place [tuples are
 * sorted lexicographically] and you can reconstruct the int64_t * edges in an embarrassingly parallel way. 
 * As I understood, the entire purpose of this function is to destroy any locality. It does not
 * rename any vertices and edges are not named anyway. 
 * For an example, think about the edge (0,1). It will eventually (at the end of kernel 1) be owned by processor P(0,0). 
 * However, assume that processor P(r1,c1) has a copy of it before the call to PermEdges. After
 * this call, some other irrelevant processor P(r2,c2) will own it. So we gained nothing, it is just a scrambled egg. 
**/
template <typename IT>
void PermEdges(DistEdgeList<IT> & DEL)
{
	IT maxedges = DEL.memedges;	// this can be optimized by calling the clean-up first
	
	// to lower memory consumption, rename in stages
	// this is not "identical" to a full randomization; 
	// but more than enough to destroy any possible locality 
	IT stages = 8;	
	IT perstage = maxedges / stages;
	
	int nproc =(DEL.commGrid)->GetSize();
	int rank = (DEL.commGrid)->GetRank();
	IT * dist = new IT[nproc];

	MTRand M;	// generate random numbers with Mersenne Twister
	for(IT s=0; s< stages; ++s)
	{
		#ifdef DEBUG
		SpParHelper::Print("PermEdges stage starting\n");	
		double st = MPI_Wtime();
		#endif
	
		IT n_sofar = s*perstage;
		IT n_thisstage = ((s==(stages-1))? (maxedges - n_sofar): perstage);

		pair<double, pair<IT,IT> >* vecpair = new pair<double, pair<IT,IT> >[n_thisstage];
		dist[rank] = n_thisstage;
		MPI_Allgather(MPI_IN_PLACE, 1, MPIType<IT>(), dist, 1, MPIType<IT>(), DEL.commGrid->GetWorld());

		for (IT i = 0; i < n_thisstage; i++)
		{
			vecpair[i].first = M.rand();
			vecpair[i].second.first = DEL.edges[2*(i+n_sofar)];
			vecpair[i].second.second = DEL.edges[2*(i+n_sofar)+1];
		}

		// less< pair<T1,T2> > works correctly (sorts w.r.t. first element of type T1)	
		SpParHelper::MemoryEfficientPSort(vecpair, n_thisstage, dist, DEL.commGrid->GetWorld());
		// SpParHelper::DebugPrintKeys(vecpair, n_thisstage, dist, DEL.commGrid->GetWorld());
		for (IT i = 0; i < n_thisstage; i++)
		{
			DEL.edges[2*(i+n_sofar)] = vecpair[i].second.first;
			DEL.edges[2*(i+n_sofar)+1] = vecpair[i].second.second;
		}
		delete [] vecpair;
		#ifdef DEBUG
		double et = MPI_Wtime();
		ostringstream timeinfo;
		timeinfo << "Stage " << s << " in " << et-st << " seconds" << endl;
		SpParHelper::Print(timeinfo.str());
		#endif
	}
	delete [] dist;
}

/**
  * Rename vertices globally. 
  *	You first need to do create a random permutation distributed on all processors. 
  *	Then the p round robin algorithm will do the renaming: 
  * For all processors P(i,i)
  *          Broadcast local_p to all p processors
  *          For j= i*N/p to min((i+1)*N/p, N)
  *                    Rename the all j's with local_p(j) inside the edgelist (and mark them
  *                    "renamed" so that yeach vertex id is renamed only once)
  **/
template <typename IU>
void RenameVertices(DistEdgeList<IU> & DEL)
{
	int nprocs = DEL.commGrid->GetSize();
	int rank = DEL.commGrid->GetRank();
	MPI_Comm World = DEL.commGrid->GetWorld(); 

	// create permutation
	FullyDistVec<IU, IU> globalPerm(DEL.commGrid);
	globalPerm.iota(DEL.getGlobalV(), 0);
	globalPerm.RandPerm();	// now, randperm can return a 0-based permutation
	IU locrows = globalPerm.MyLocLength(); 
	
	// way to mark whether each vertex was already renamed or not
	IU locedgelist = 2*DEL.getNumLocalEdges();
	bool* renamed = new bool[locedgelist];
	fill_n(renamed, locedgelist, 0);
	
	// permutation for one round
	IU * localPerm = NULL;
	IU permsize;
	IU startInd = 0;

	//vector < pair<IU, IU> > vec;
	//for(IU i=0; i< DEL.getNumLocalEdges(); i++)
	//	vec.push_back(make_pair(DEL.edges[2*i], DEL.edges[2*i+1]));
	//sort(vec.begin(), vec.end());
	//vector < pair<IU, IU> > uniqued;
	//unique_copy(vec.begin(), vec.end(), back_inserter(uniqued));
	//cout << "before: " << vec.size() << " and after: " << uniqued.size() << endl;
	
	for (int round = 0; round < nprocs; round++)
	{
		// broadcast the permutation from the one processor
		if (rank == round)
		{
			permsize = locrows;
			localPerm = new IU[permsize];
			copy(globalPerm.arr.begin(), globalPerm.arr.end(), localPerm);
		}
		MPI_Bcast(&permsize, 1, MPIType<IU>(), round, World);
		if(rank != round)
		{
			localPerm = new IU[permsize];
		}
		MPI_Bcast(localPerm, permsize, MPIType<IU>(), round, World);
	
		// iterate over 	
		for (typename vector<IU>::size_type j = 0; j < (unsigned)locedgelist ; j++)
		{
			// We are renaming vertices, not edges
			if (startInd <= DEL.edges[j] && DEL.edges[j] < (startInd + permsize) && !renamed[j])
			{
				DEL.edges[j] = localPerm[DEL.edges[j]-startInd];
				renamed[j] = true;
			}
		}
		startInd += permsize;
		delete [] localPerm;
	}
	delete [] renamed;
}

