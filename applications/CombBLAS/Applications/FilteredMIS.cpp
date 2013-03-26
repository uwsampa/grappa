#define DETERMINISTIC
#include "../CombBLAS.h"
#include <mpi.h>
#include <sys/time.h>
#include <iostream>
#include <functional>
#include <algorithm>
#include <vector>
#include <string>
#include <sstream>
#ifdef THREADED
	#ifndef _OPENMP
	#define _OPENMP
	#endif
	#include <omp.h>
#endif



double cblas_alltoalltime;
double cblas_allgathertime;
#ifdef _OPENMP
int cblas_splits = omp_get_max_threads();
#else
int cblas_splits = 1;
#endif

#include "TwitterEdge.h"

#define EDGEFACTOR 5 // For MIS
#define ITERS 16
#define PERCENTS 4  // testing with 4 different percentiles

using namespace std;


template <typename PARMAT>
void Symmetricize(PARMAT & A)
{
	// boolean addition is practically a "logical or"
	// therefore this doesn't destruct any links
	PARMAT AT = A;
	AT.Transpose();
	A += AT;
}


struct DetSymmetricize: public std::binary_function<TwitterEdge, TwitterEdge, TwitterEdge>
{
	// have to deterministically choose between one of the two values.
	// cannot just add them because that will change the distribution (small values are unlikely to survive)
	TwitterEdge operator()(const TwitterEdge & g, const TwitterEdge & t)
	{
		TwitterEdge toret = g;

		if(((g.latest + t.latest) & 1) == 1)
		{
			toret.latest = std::min(g.latest, t.latest);
		}
		else
		{
			toret.latest = std::max(g.latest, t.latest);
		}
		return toret;
	}
};

typedef SpParMat < int64_t, TwitterEdge, SpDCCols<int64_t, TwitterEdge > > PSpMat_Twitter;
typedef SpParMat < int64_t, bool, SpDCCols<int64_t, bool > > PSpMat_Bool;

void SymmetricizeRands(PSpMat_Twitter & A)
{
	PSpMat_Twitter AT = A;
	AT.Transpose();
	// SpParMat<IU,RETT,RETDER> EWiseApply (const SpParMat<IU,NU1,UDERA> & A,
	// const SpParMat<IU,NU2,UDERB> & B, _BinaryOperation __binary_op, bool notB, const NU2& defaultBVal)
	// Default B value is irrelevant since the structures of the matrices are
	A = EWiseApply<TwitterEdge, SpDCCols<int64_t, TwitterEdge > >(A, AT, DetSymmetricize(), false, TwitterEdge());
}

#ifdef DETERMINISTIC
        MTRand GlobalMT(1);
#else
        MTRand GlobalMT;
#endif
struct Twitter_obj_randomizer : public std::unary_function<TwitterEdge, TwitterEdge>
{
  const TwitterEdge operator()(const TwitterEdge & x) const
  {
	short mycount = 1;
	bool myfollow = 0;
	time_t mylatest = static_cast<int64_t>(GlobalMT.rand() * 10000);	// random.randrange(0,10000)

	return TwitterEdge(mycount, myfollow, mylatest);
  }
};

struct Twitter_materialize: public std::binary_function<TwitterEdge, time_t, bool>
{
	bool operator()(const TwitterEdge & x, time_t sincedate) const
	{
		if(x.isRetwitter() && x.LastTweetBy(sincedate))
			return false;	// false if the edge is going to be kept
		else
			return true;	// true if the edge is to be pruned
	}
};

//  def rand( verc ):
//	import random
//	return random.random()
struct randGen : public std::unary_function<double, double>
{
	const double operator()(const double & ignore)
	{
		return GlobalMT.rand();
	}
};



int main(int argc, char* argv[])
{

	int nprocs, myrank;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD,&nprocs);
	MPI_Comm_rank(MPI_COMM_WORLD,&myrank);
	if(argc < 2)
	{
		if(myrank == 0)
		{
			cout << "Usage: ./FilteredMIS <Scale>" << endl;
		}
		MPI_Finalize();
		return -1;
	}
	{
		// Declare objects
		PSpMat_Twitter A;
		FullyDistVec<int64_t, int64_t> indegrees;	// in-degrees of vertices (including multi-edges and self-loops)
		FullyDistVec<int64_t, int64_t> oudegrees;	// out-degrees of vertices (including multi-edges and self-loops)
		FullyDistVec<int64_t, int64_t> degrees;	// combined degrees of vertices (including multi-edges and self-loops)
		PSpMat_Bool * ABool;

		SpParHelper::Print("Using synthetic data, which we ALWAYS permute for load balance\n");
		SpParHelper::Print("We only balance the original input, we don't repermute after each filter change\n");
		SpParHelper::Print("BFS is run on UNDIRECTED graph, hence hitting CCs, and TEPS is bidirectional\n");

		double initiator[4] = {.25, .25, .25, .25};	// creating erdos-renyi
		double t01 = MPI_Wtime();
		DistEdgeList<int64_t> * DEL = new DistEdgeList<int64_t>();

		unsigned scale = static_cast<unsigned>(atoi(argv[1]));
		ostringstream outs;
		outs << "Forcing scale to : " << scale << endl;
		SpParHelper::Print(outs.str());

		// parameters: (double initiator[4], int log_numverts, int edgefactor, bool scramble, bool packed)
		DEL->GenGraph500Data(initiator, scale, EDGEFACTOR, true, true );	// generate packed edges
		SpParHelper::Print("Generated renamed edge lists\n");

		ABool = new PSpMat_Bool(*DEL, false);
		int64_t removed  = ABool->RemoveLoops();
		ostringstream loopinfo;
		loopinfo << "Converted to Boolean and removed " << removed << " loops" << endl;
		SpParHelper::Print(loopinfo.str());
		ABool->PrintInfo();
		delete DEL;	// free memory
		A = PSpMat_Twitter(*ABool); // any upcasting generates the default object

		double t02 = MPI_Wtime();
		ostringstream tinfo;
		tinfo << "Generation took " << t02-t01 << " seconds" << endl;
		SpParHelper::Print(tinfo.str());

		// indegrees is sum along rows because A is loaded as "tranposed", similarly oudegrees is sum along columns
		ABool->PrintInfo();
		ABool->Reduce(oudegrees, Column, plus<int64_t>(), static_cast<int64_t>(0));
		ABool->Reduce(indegrees, Row, plus<int64_t>(), static_cast<int64_t>(0));

		// indegrees_filt and oudegrees_filt is used for the real data
		FullyDistVec<int64_t, int64_t> indegrees_filt;
		FullyDistVec<int64_t, int64_t> oudegrees_filt;
		FullyDistVec<int64_t, int64_t> degrees_filt[4];	// used for the synthetic data (symmetricized before randomization)
		int64_t keep[PERCENTS] = {100, 1000, 2500, 10000}; 	// ratio of edges kept in range (0, 10000)

		degrees = indegrees;
		degrees.EWiseApply(oudegrees, plus<int64_t>());
		SpParHelper::Print("All degrees calculated\n");
		delete ABool;

		float balance = A.LoadImbalance();
		ostringstream outlb;
		outlb << "Load balance: " << balance << endl;
		SpParHelper::Print(outlb.str());

		// We symmetricize before we apply the random generator
		// Otherwise += will naturally add the random numbers together
		// hence will create artificially high-permeable filters
		Symmetricize(A);	// A += A';
		SpParHelper::Print("Symmetricized\n");

		A.Apply(Twitter_obj_randomizer());
		A.PrintInfo();
		SymmetricizeRands(A);
		SpParHelper::Print("Symmetricized Rands\n");
		A.PrintInfo();


		FullyDistVec<int64_t, int64_t> * nonisov = new FullyDistVec<int64_t, int64_t>(degrees.FindInds(bind2nd(greater<int64_t>(), 0)));
		SpParHelper::Print("Found (and permuted) non-isolated vertices\n");
		nonisov->RandPerm();	// so that A(v,v) is load-balanced (both memory and time wise)
		A(*nonisov, *nonisov, true);	// in-place permute to save memory
		SpParHelper::Print("Dropped isolated vertices from input\n");

		indegrees = indegrees(*nonisov);	// fix the degrees arrays too
		oudegrees = oudegrees(*nonisov);
		degrees = degrees(*nonisov);
		delete nonisov;

		for (int i=0; i < PERCENTS; i++)
		{
			PSpMat_Twitter B = A;
			B.Prune(bind2nd(Twitter_materialize(), keep[i]));
			PSpMat_Bool BBool = B;
			BBool.PrintInfo();
			float balance = B.LoadImbalance();
			ostringstream outs;
			outs << "Load balance of " << static_cast<float>(keep[i])/100 << "% filtered case: " << balance << endl;
			SpParHelper::Print(outs.str());

			// degrees_filt[i] is by-default generated as permuted
			BBool.Reduce(degrees_filt[i], Column, plus<int64_t>(), static_cast<int64_t>(0));  // Column=Row since BBool is symmetric
		}

		float balance_former = A.LoadImbalance();
		ostringstream outs_former;
		outs_former << "Load balance: " << balance_former << endl;
		SpParHelper::Print(outs_former.str());

		for(int trials =0; trials < PERCENTS; trials++)
		{
			cblas_allgathertime = 0;
			cblas_alltoalltime = 0;
			double MISVS[ITERS]; // numbers of vertices in each MIS
			double TIMES[ITERS];

			LatestRetwitterMIS::sincedate = keep[trials];
			LatestRetwitterSelect2nd::sincedate = keep[trials];
			ostringstream outs;
			outs << "Initializing since date (only once) to " << LatestRetwitterMIS::sincedate << endl;
			SpParHelper::Print(outs.str());

			for(int sruns = 0; sruns < ITERS; ++sruns)
			{
				double t1 = MPI_Wtime();
				int64_t nvert = A.getncol();

				//# the final result set. S[i] exists and is 1 if vertex i is in the MIS
				//S = Vec(nvert, sparse=True)
				FullyDistSpVec<int64_t, uint8_t> S ( A.getcommgrid(), nvert);

				//# the candidate set. initially all vertices are candidates.
				//# this vector doubles as 'r', the random value vector.
				//# i.e. if C[i] exists, then i is a candidate. The value C[i] is i's r for this iteration.
				//C = Vec.ones(nvert, sparse=True)
				//FullyDistSpVec's length is not the same as its nnz
				//Since FullyDistSpVec::Apply only affects nonzeros, nnz should be forced to glen
				// FullyDistVec ( shared_ptr<CommGrid> grid, IT globallen, NT initval);
				FullyDistVec<int64_t, double> * denseC = new FullyDistVec<int64_t, double>( A.getcommgrid(), nvert, 1.0);
				FullyDistSpVec<int64_t, double> C ( *denseC);
				delete denseC;

				FullyDistSpVec<int64_t, double> min_neighbor_r ( A.getcommgrid(), nvert);
				FullyDistSpVec<int64_t, uint8_t> new_S_members ( A.getcommgrid(), nvert);
				FullyDistSpVec<int64_t, uint8_t> new_S_neighbors ( A.getcommgrid(), nvert);

				while (C.getnnz() > 0)
				{
					//# label each vertex in C with a random value
					C.Apply(randGen());

					//# find the smallest random value among a vertex's neighbors
					//# In other words: min_neighbor_r[i] = min(C[j] for all neighbors j of vertex i)
					//min_neighbor_r = Gmatrix.SpMV(C, sr(myMin,select2nd)) # could use "min" directly
					SpMV<LatestRetwitterMIS>(A, C, min_neighbor_r, false);	// min_neighbor_r empty OK?
					#ifdef PRINTITERS
					min_neighbor_r.PrintInfo("Neighbors");
					#endif

					#ifdef DEBUG
					min_neighbor_r.DebugPrint();
					C.DebugPrint();
					#endif

					//# The vertices to be added to S this iteration are those whose random value is
					//# smaller than those of all its neighbors:
					//# new_S_members[i] exists if C[i] < min_neighbor_r[i]
					//# If C[i] exists and min_neighbor_r[i] doesn't, still a value is returned with bin_op(NULL,C[i])
					//new_S_members = min_neighbor_r.eWiseApply(C, return1, doOp=is2ndSmaller, allowANulls=True, allowBNulls=False, inPlace=False, ANull=2)
					new_S_members = EWiseApply<uint8_t>(min_neighbor_r, C, return1_uint8(), is2ndSmaller(), true, false, (double) 2.0,  (double) 2.0, true);
					//// template <typename RET, typename IU ...>
					//// FullyDistSpVec<IU,RET> EWiseApply (const FullyDistSpVec<IU,NU1> & V, const FullyDistSpVec<IU,NU2> & W , _BinaryOperation _binary_op, _BinaryPredicate _doOp,
					//// bool allowVNulls, bool allowWNulls, NU1 Vzero, NU2 Wzero, const bool allowIntersect = true);
					#ifdef PRINTITERS
					new_S_members.PrintInfo("New members of the MIS");
					#endif

					#ifdef DEBUG
					new_S_members.DebugPrint();
					#endif

					//# new_S_members are no longer candidates, so remove them from C
					//C.eWiseApply(new_S_members, return1, allowANulls=False, allowIntersect=False, allowBNulls=True, inPlace=True)
					C = EWiseApply<double>(C, new_S_members, return1_uint8(), return1_uint8(), false, true, (double) 0.0, (uint8_t) 0, false);
					#ifdef PRINTITERS
					C.PrintInfo("Entries to be removed from the Candidates set");
					#endif

					//# find neighbors of new_S_members
					//new_S_neighbors = Gmatrix.SpMV(new_S_members, sr(select2nd,select2nd))
					SpMV<LatestRetwitterSelect2nd>(A, new_S_members, new_S_neighbors, false);

					//# remove neighbors of new_S_members from C, because they cannot be part of the MIS anymore
					//# If C[i] exists and new_S_neighbors[i] doesn't, still a value is returned with bin_op(NULL,C[i])
					//C.eWiseApply(new_S_neighbors, return1, allowANulls=False, allowIntersect=False, allowBNulls=True, inPlace=True)
					C = EWiseApply<double>(C, new_S_neighbors, return1_uint8(), return1_uint8(), false, true, (double) 0.0, (uint8_t) 0, false);
					#ifdef PRINTITERS
					C.PrintInfo("Candidates set after neighbors of MIS removed");
					#endif

					//# add new_S_members to S
					//S.eWiseApply(new_S_members, return1, allowANulls=True, allowBNulls=True, inPlace=True)
					S = EWiseApply<uint8_t>(S, new_S_members, return1_uint8(), return1_uint8(), true, true, (uint8_t) 1, (uint8_t) 1, true);
					S.PrintInfo("The current MIS:");
				}
				double t2 = MPI_Wtime();

				ostringstream ositr;
				ositr << "MIS has " << S.getnnz() << " vertices" << endl;
				SpParHelper::Print(ositr.str());

				ostringstream ositr2;
				ositr << "MIS time: " << t2-t1 << " seconds" << endl;
				SpParHelper::Print(ositr.str());

				TIMES[sruns] = t2-t1;
				MISVS[sruns] = S.getnnz();
			} // end for(int sruns = 0; sruns < ITERS; ++sruns)

			ostringstream os;
			os << "Per iteration communication times: " << endl;
			os << "AllGatherv: " << cblas_allgathertime / ITERS << endl;
			os << "AlltoAllv: " << cblas_alltoalltime / ITERS << endl;

			sort(MISVS, MISVS+ITERS);
			os << "--------------------------" << endl;
			os << "Min MIS vertices: " << MISVS[0] << endl;
			os << "Median MIS vertices: " << (MISVS[(ITERS/2)-1] + MISVS[ITERS/2])/2 << endl;
			os << "Max MIS vertices: " << MISVS[ITERS-1] << endl;
			double mean = accumulate( MISVS, MISVS+ITERS, 0.0 )/ ITERS;
			vector<double> zero_mean(ITERS);	// find distances to the mean
			transform(MISVS, MISVS+ITERS, zero_mean.begin(), bind2nd( minus<double>(), mean ));
			// self inner-product is sum of sum of squares
			double deviation = inner_product( zero_mean.begin(),zero_mean.end(), zero_mean.begin(), 0.0 );
			deviation = sqrt( deviation / (ITERS-1) );
			os << "Mean MIS vertices: " << mean << endl;
			os << "STDDEV MIS vertices: " << deviation << endl;
			os << "--------------------------" << endl;

			sort(TIMES,TIMES+ITERS);
			os << "Filter keeps " << static_cast<double>(keep[trials])/100.0 << " percentage of edges" << endl;
			os << "Min time: " << TIMES[0] << " seconds" << endl;
			os << "Median time: " << (TIMES[(ITERS/2)-1] + TIMES[ITERS/2])/2 << " seconds" << endl;
			os << "Max time: " << TIMES[ITERS-1] << " seconds" << endl;
			mean = accumulate( TIMES, TIMES+ITERS, 0.0 )/ ITERS;
			transform(TIMES, TIMES+ITERS, zero_mean.begin(), bind2nd( minus<double>(), mean ));
			deviation = inner_product( zero_mean.begin(),zero_mean.end(), zero_mean.begin(), 0.0 );
			deviation = sqrt( deviation / (ITERS-1) );
			os << "Mean time: " << mean << " seconds" << endl;
			os << "STDDEV time: " << deviation << " seconds" << endl;
			os << "--------------------------" << endl;
			SpParHelper::Print(os.str());
		}	// end for(int trials =0; trials < PERCENTS; trials++)
	}
	MPI_Finalize();
	return 0;
}

