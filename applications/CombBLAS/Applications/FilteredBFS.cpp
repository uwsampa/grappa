/****************************************************************/
/* Parallel Combinatorial BLAS Library (for Graph Computations) */
/* version 1.3 -------------------------------------------------*/
/* date: 2/1/2013 ----------------------------------------------*/
/* authors: Aydin Buluc (abuluc@lbl.gov), Adam Lugowski --------*/
/****************************************************************/
/*
 Copyright (c) 2010-, Aydin Buluc
 
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
#ifdef TAU_PROFILE
	#include <Profile/Profiler.h>
#endif


double cblas_alltoalltime;
double cblas_allgathertime;
#ifdef _OPENMP
int cblas_splits = omp_get_max_threads(); 
#else
int cblas_splits = 1;
#endif

#include "TwitterEdge.h"

#define MAX_ITERS 20000
#define EDGEFACTOR 16
#define ITERS 16 
#define CC_LIMIT 100
#define PERCENTS 4  // testing with 4 different percentiles
#define MINRUNS 4
//#define ONLYTIME // don't calculate TEPS

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

int main(int argc, char* argv[])
{
#ifdef TAU_PROFILE
	TAU_PROFILE_TIMER(maintimer, "main()", "int (int, char **)", TAU_DEFAULT);
    	TAU_PROFILE_INIT(argc, argv);
    	TAU_PROFILE_START(maintimer);
#endif
	int nprocs, myrank;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD,&nprocs);
	MPI_Comm_rank(MPI_COMM_WORLD,&myrank);
	int MAXTRIALS;
	if(argc < 3)
	{
		if(myrank == 0)
		{
			cout << "Usage: ./FilteredBFS <File, Gen> <Input Name | Scale> (Optional: Double)" << endl;
			cout << "Example: ./FilteredBFS File twitter_small.txt Double" << endl;
		}
		MPI_Finalize();
		return -1;
	}		
	{
		typedef SpParMat < int64_t, TwitterEdge, SpDCCols<int64_t, TwitterEdge > > PSpMat_Twitter;
		typedef SpParMat < int64_t, bool, SpDCCols<int64_t, bool > > PSpMat_Bool;

		// Declare objects
		PSpMat_Twitter A;	
		FullyDistVec<int64_t, int64_t> indegrees;	// in-degrees of vertices (including multi-edges and self-loops)
		FullyDistVec<int64_t, int64_t> oudegrees;	// out-degrees of vertices (including multi-edges and self-loops)
		FullyDistVec<int64_t, int64_t> degrees;	// combined degrees of vertices (including multi-edges and self-loops)
		PSpMat_Bool * ABool;

		double t01 = MPI_Wtime();
		if(string(argv[1]) == string("File")) // text|binary input option
		{
			SpParHelper::Print("Using real data, which we NEVER permute for load balance, also leaving isolated vertices as-is, if any\n");
			SpParHelper::Print("This is because the input is assumed to be load balanced\n");
			SpParHelper::Print("BFS is run on DIRECTED graph, hence hitting SCCs, and TEPS is unidirectional\n");

			// ReadDistribute (const string & filename, int master, bool nonum, HANDLER handler, bool transpose, bool pario)
			// if nonum is true, then numerics are not supplied and they are assumed to be all 1's
			A.ReadDistribute(string(argv[2]), 0, false, TwitterReadSaveHandler<int64_t>(), true, true);	// read it from file (and transpose on the fly)

			A.PrintInfo();
			SpParHelper::Print("Read input\n");

			ABool = new PSpMat_Bool(A);
			if(argc == 4 && string(argv[3]) == string("Double"))
			{
				MAXTRIALS = 2;
			}
			else 
			{
				MAXTRIALS = 1;
			}
		}
		else if(string(argv[1]) == string("Gen"))
		{
			SpParHelper::Print("Using synthetic data, which we ALWAYS permute for load balance\n");
			SpParHelper::Print("We only balance the original input, we don't repermute after each filter change\n");
			SpParHelper::Print("BFS is run on UNDIRECTED graph, hence hitting CCs, and TEPS is bidirectional\n");

 			double initiator[4] = {.57, .19, .19, .05};
			double t01 = MPI_Wtime();
			double t02;
			DistEdgeList<int64_t> * DEL = new DistEdgeList<int64_t>();

			unsigned scale = static_cast<unsigned>(atoi(argv[2]));
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
			
			MAXTRIALS = PERCENTS;	// benchmarking
		}
		else 
		{	
			SpParHelper::Print("Not supported yet\n");
			return 0;
		}
		double t02 = MPI_Wtime();			
		ostringstream tinfo;
		tinfo << "I/O (or generation) took " << t02-t01 << " seconds" << endl;                
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
		
		if(string(argv[1]) == string("File"))	// if using synthetic data, no notion of out/in degrees after randomization exist
		{
			struct tm timeinfo;
			memset(&timeinfo, 0, sizeof(struct tm));
			int year, month, day, hour, min, sec;
			year = 2009;	month = 7;	day = 1;
			hour = 0;	min = 0;	sec = 0;
			
			timeinfo.tm_year = year - 1900; // year is "years since 1900"
			timeinfo.tm_mon = month - 1 ;   // month is in range 0...11
			timeinfo.tm_mday = day;         // range 1...31
			timeinfo.tm_hour = hour;        // range 0...23
			timeinfo.tm_min = min;          // range 0...59
			timeinfo.tm_sec = sec;          // range 0.
			time_t mysincedate = timegm(&timeinfo);
			
			PSpMat_Twitter B = A;
			B.Prune(bind2nd(Twitter_materialize(), mysincedate));
			PSpMat_Bool BBool = B;
			BBool.PrintInfo();
			BBool.Reduce(oudegrees_filt, Column, plus<int64_t>(), static_cast<int64_t>(0)); 	
			BBool.Reduce(indegrees_filt, Row, plus<int64_t>(), static_cast<int64_t>(0)); 
		}

		degrees = indegrees;	
		degrees.EWiseApply(oudegrees, plus<int64_t>());
		SpParHelper::Print("All degrees calculated\n");
		delete ABool;

		float balance = A.LoadImbalance();
		ostringstream outs;
		outs << "Load balance: " << balance << endl;
		SpParHelper::Print(outs.str());

		if(string(argv[1]) == string("Gen"))
		{
			// We symmetricize before we apply the random generator
			// Otherwise += will naturally add the random numbers together
			// hence will create artificially high-permeable filters
			Symmetricize(A);	// A += A';
			SpParHelper::Print("Symmetricized\n");	

			A.Apply(Twitter_obj_randomizer());
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
		}
		// real data is pre-balanced, because otherwise even the load balancing routine crushes due to load imbalance
			
		float balance_former = A.LoadImbalance();
		ostringstream outs_former;
		outs_former << "Load balance: " << balance_former << endl;
		SpParHelper::Print(outs_former.str());

		MPI_Barrier(MPI_COMM_WORLD);
		double t1 = MPI_Wtime();

		FullyDistVec<int64_t, int64_t> Cands(MAX_ITERS);
		double nver = (double) degrees.TotalLength();
		Cands.SelectCandidates(nver, true);

		for(int trials =0; trials < MAXTRIALS; trials++)	
		{
			if(string(argv[1]) == string("Gen"))
			{
				LatestRetwitterBFS::sincedate = keep[trials];
				ostringstream outs;
				outs << "Initializing since date (only once) to " << LatestRetwitterBFS::sincedate << endl;
				SpParHelper::Print(outs.str());
			}

			cblas_allgathertime = 0;
			cblas_alltoalltime = 0;

			double MTEPS[ITERS]; double INVMTEPS[ITERS]; double TIMES[ITERS]; double EDGES[ITERS];
			double MPEPS[ITERS]; double INVMPEPS[ITERS];
			int sruns = 0;		// successful runs
			for(int i=0; i<MAX_ITERS && sruns < ITERS; ++i)
			{
				// FullyDistVec ( shared_ptr<CommGrid> grid, IT globallen, NT initval);
				FullyDistVec<int64_t, ParentType> parents ( A.getcommgrid(), A.getncol(), ParentType());	

				// FullyDistSpVec ( shared_ptr<CommGrid> grid, IT glen);
				FullyDistSpVec<int64_t, ParentType> fringe(A.getcommgrid(), A.getncol());	

				MPI_Barrier(MPI_COMM_WORLD);
				double t1 = MPI_Wtime();

				fringe.SetElement(Cands[i], Cands[i]);
				parents.SetElement(Cands[i], ParentType(Cands[i]));	// make root discovered
				int iterations = 0;
				while(fringe.getnnz() > 0)
				{
					fringe.ApplyInd(NumSetter);

					// SpMV with sparse vector, optimizations disabled for generality
					SpMV<LatestRetwitterBFS>(A, fringe, fringe, false);	
					
					//  EWiseApply (const FullyDistSpVec<IU,NU1> & V, const FullyDistVec<IU,NU2> & W, 
					//		_BinaryOperation _binary_op, _BinaryPredicate _doOp, bool allowVNulls, NU1 Vzero)
					fringe = EWiseApply<ParentType>(fringe, parents, getfringe(), keepinfrontier_f(), true, ParentType());
					parents += fringe;
					iterations++;
					
				}
				MPI_Barrier(MPI_COMM_WORLD);
				double t2 = MPI_Wtime();
	
			
				FullyDistSpVec<int64_t, ParentType> parentsp = parents.Find(isparentset());
				parentsp.Apply(myset<ParentType>(ParentType(1)));

#ifndef ONLYTIME
				FullyDistSpVec<int64_t, int64_t> intraversed, inprocessed, outraversed, ouprocessed;
				inprocessed = EWiseApply<int64_t>(parentsp, indegrees, seldegree(), passifthere(), true, ParentType());
				ouprocessed = EWiseApply<int64_t>(parentsp, oudegrees, seldegree(), passifthere(), true, ParentType());
				int64_t nedges, in_nedges, ou_nedges;
				if(string(argv[1]) == string("Gen"))
				{
					FullyDistSpVec<int64_t, int64_t> traversed = EWiseApply<int64_t>(parentsp, degrees_filt[trials], seldegree(), passifthere(), true, ParentType());
					nedges = traversed.Reduce(plus<int64_t>(), (int64_t) 0);
				}
				else 
				{
					intraversed = EWiseApply<int64_t>(parentsp, indegrees_filt, seldegree(), passifthere(), true, ParentType());
					outraversed = EWiseApply<int64_t>(parentsp, oudegrees_filt, seldegree(), passifthere(), true, ParentType());
					in_nedges = intraversed.Reduce(plus<int64_t>(), (int64_t) 0);
					ou_nedges = outraversed.Reduce(plus<int64_t>(), (int64_t) 0);
					nedges = in_nedges + ou_nedges;	// count birectional edges twice
				}
				
				int64_t in_nedges_processed = inprocessed.Reduce(plus<int64_t>(), (int64_t) 0);
				int64_t ou_nedges_processed = ouprocessed.Reduce(plus<int64_t>(), (int64_t) 0);
				int64_t nedges_processed = in_nedges_processed + ou_nedges_processed;	// count birectional edges twice
#else
				int64_t in_nedges, ou_nedges, nedges, in_nedges_processed, ou_nedges_processed, nedges_processed = 0;
#endif

				if(parentsp.getnnz() > CC_LIMIT)
				{
					// intraversed.PrintInfo("Incoming edges traversed per vertex");
					// intraversed.DebugPrint();
					// outraversed.PrintInfo("Outgoing edges traversed per vertex");
					// outraversed.DebugPrint();
					
				#ifdef DEBUG
					parents.PrintInfo("Final parents array");
					parents.DebugPrint();
				#endif
					
					ostringstream outnew;  
					outnew << i << "th starting vertex was " << Cands[i] << endl;
					outnew << "Number iterations: " << iterations << endl;
					outnew << "Number of vertices found: " << parentsp.getnnz() << endl; 
					outnew << "Number of edges traversed in both directions: " << nedges << endl;
					if(string(argv[1]) == string("File"))
						outnew << "Number of edges traversed in one direction: " << ou_nedges << endl;
					outnew << "Number of edges processed in both directions: " << nedges_processed << endl;
					outnew << "Number of edges processed in one direction: " << ou_nedges_processed << endl;
					outnew << "BFS time: " << t2-t1 << " seconds" << endl;
					outnew << "MTEPS (bidirectional): " << static_cast<double>(nedges) / (t2-t1) / 1000000.0 << endl;
					if(string(argv[1]) == string("File"))
						outnew << "MTEPS (unidirectional): " << static_cast<double>(ou_nedges) / (t2-t1) / 1000000.0 << endl;
					outnew << "MPEPS (bidirectional): " << static_cast<double>(nedges_processed) / (t2-t1) / 1000000.0 << endl;
					outnew << "MPEPS (unidirectional): " << static_cast<double>(ou_nedges_processed) / (t2-t1) / 1000000.0 << endl;
					outnew << "Total communication (average so far): " << (cblas_allgathertime + cblas_alltoalltime) / (i+1) << endl;

					TIMES[sruns] = t2-t1;
					if(string(argv[1]) == string("Gen"))
						EDGES[sruns] = static_cast<double>(nedges);
					else
						EDGES[sruns] = static_cast<double>(ou_nedges);

					MTEPS[sruns] = EDGES[sruns] / (t2-t1) / 1000000.0;
					MPEPS[sruns++] = static_cast<double>(nedges_processed) / (t2-t1) / 1000000.0;
					SpParHelper::Print(outnew.str());
				}
			}
			if (sruns < MINRUNS)
			{
				SpParHelper::Print("Not enough valid runs done\n");
				MPI_Finalize();
				return 0;
			}
			ostringstream os;
			
			os << sruns << " valid runs done" << endl;
			os << "Connected component lower limite was " << CC_LIMIT << endl;
			os << "Per iteration communication times: " << endl;
			os << "AllGatherv: " << cblas_allgathertime / sruns << endl;
			os << "AlltoAllv: " << cblas_alltoalltime / sruns << endl;

			sort(EDGES, EDGES+sruns);
			os << "--------------------------" << endl;
			os << "Min nedges: " << EDGES[0] << endl;
			os << "Median nedges: " << (EDGES[(sruns/2)-1] + EDGES[sruns/2])/2 << endl;
			os << "Max nedges: " << EDGES[sruns-1] << endl;
 			double mean = accumulate( EDGES, EDGES+sruns, 0.0 )/ sruns;
			vector<double> zero_mean(sruns);	// find distances to the mean
			transform(EDGES, EDGES+sruns, zero_mean.begin(), bind2nd( minus<double>(), mean )); 	
			// self inner-product is sum of sum of squares
			double deviation = inner_product( zero_mean.begin(),zero_mean.end(), zero_mean.begin(), 0.0 );
   			deviation = sqrt( deviation / (sruns-1) );
   			os << "Mean nedges: " << mean << endl;
			os << "STDDEV nedges: " << deviation << endl;
			os << "--------------------------" << endl;
	
			sort(TIMES,TIMES+sruns);
			os << "Filter keeps " << static_cast<double>(keep[trials])/100.0 << " percentage of edges" << endl;
			os << "Min time: " << TIMES[0] << " seconds" << endl;
			os << "Median time: " << (TIMES[(sruns/2)-1] + TIMES[sruns/2])/2 << " seconds" << endl;
			os << "Max time: " << TIMES[sruns-1] << " seconds" << endl;
 			mean = accumulate( TIMES, TIMES+sruns, 0.0 )/ sruns;
			transform(TIMES, TIMES+sruns, zero_mean.begin(), bind2nd( minus<double>(), mean )); 	
			deviation = inner_product( zero_mean.begin(),zero_mean.end(), zero_mean.begin(), 0.0 );
   			deviation = sqrt( deviation / (sruns-1) );
   			os << "Mean time: " << mean << " seconds" << endl;
			os << "STDDEV time: " << deviation << " seconds" << endl;
			os << "--------------------------" << endl;

			sort(MTEPS, MTEPS+sruns);
			os << "Min MTEPS: " << MTEPS[0] << endl;
			os << "Median MTEPS: " << (MTEPS[(sruns/2)-1] + MTEPS[sruns/2])/2 << endl;
			os << "Max MTEPS: " << MTEPS[sruns-1] << endl;
			transform(MTEPS, MTEPS+sruns, INVMTEPS, safemultinv<double>()); 	// returns inf for zero teps
			double hteps = static_cast<double>(sruns) / accumulate(INVMTEPS, INVMTEPS+sruns, 0.0);	
			os << "Harmonic mean of MTEPS: " << hteps << endl;
			transform(INVMTEPS, INVMTEPS+sruns, zero_mean.begin(), bind2nd(minus<double>(), 1/hteps));
			deviation = inner_product( zero_mean.begin(),zero_mean.end(), zero_mean.begin(), 0.0 );
   			deviation = sqrt( deviation / (sruns-1) ) * (hteps*hteps);	// harmonic_std_dev
			os << "Harmonic standard deviation of MTEPS: " << deviation << endl;

			sort(MPEPS, MPEPS+sruns);
			os << "Bidirectional Processed Edges per second (to estimate sustained BW)"<< endl;
			os << "Min MPEPS: " << MPEPS[0] << endl;
			os << "Median MPEPS: " << (MPEPS[(sruns/2)-1] + MPEPS[sruns/2])/2 << endl;
			os << "Max MPEPS: " << MPEPS[sruns-1] << endl;
			transform(MPEPS, MPEPS+sruns, INVMPEPS, safemultinv<double>()); 	// returns inf for zero teps
			double hpeps = static_cast<double>(sruns) / accumulate(INVMPEPS, INVMPEPS+sruns, 0.0);	
			os << "Harmonic mean of MPEPS: " << hpeps << endl;
			transform(INVMPEPS, INVMPEPS+sruns, zero_mean.begin(), bind2nd(minus<double>(), 1/hpeps));
			deviation = inner_product( zero_mean.begin(),zero_mean.end(), zero_mean.begin(), 0.0 );
   			deviation = sqrt( deviation / (sruns-1) ) * (hpeps*hpeps);	// harmonic_std_dev
			os << "Harmonic standard deviation of MPEPS: " << deviation << endl;
			SpParHelper::Print(os.str());
		}
	}
#ifdef TAU_PROFILE
    	TAU_PROFILE_STOP(maintimer);
#endif
	MPI_Finalize();
	return 0;
}

