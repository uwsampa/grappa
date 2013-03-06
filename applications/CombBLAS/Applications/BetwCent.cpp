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

// These macros should be defined before stdint.h is included
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#include <stdint.h>
#include <mpi.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>  // Required for stringstreams
#include <ctime>
#include <cmath>
#include "../CombBLAS.h"
#include "../SpParVec.h"	// ABAB: deprecated
#include "../DenseParVec.h"	// ABAB: deprecated


// Simple helper class for declarations: Just the numerical type is templated 
// The index type and the sequential matrix type stays the same for the whole code
// In this case, they are "int" and "SpDCCols"
template <class NT>
class Dist 
{ 
public: 
	typedef SpDCCols < int, NT > DCCols;
	typedef SpParMat < int, NT, DCCols > MPI_DCCols;
};


int main(int argc, char* argv[])
{
	int nprocs, myrank;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD,&nprocs);
	MPI_Comm_rank(MPI_COMM_WORLD,&myrank);
	
	typedef PlusTimesSRing<bool, int> PTBOOLINT;	
	typedef PlusTimesSRing<bool, double> PTBOOLDOUBLE;

	if(argc < 4)
        {
		if(myrank == 0)
		{	
                	cout << "Usage: ./betwcent <BASEADDRESS> <K4APPROX> <BATCHSIZE>" << endl;
                	cout << "Example: ./betwcent Data/ 15 128" << endl;
                	cout << "Input file input.txt should be under <BASEADDRESS> in triples format" << endl;
                	cout << "<BATCHSIZE> should be a multiple of sqrt(p)" << endl;
			cout << "Because <BATCHSIZE> is for the overall matrix (similarly, <K4APPROX> is global as well) " << endl;
 		}
		MPI_Finalize();
		return -1;
        }

	{
		int K4Approx = atoi(argv[2]);
		int batchSize = atoi(argv[3]);

		string directory(argv[1]);		
		string ifilename = "input.txt";
		ifilename = directory+"/"+ifilename;

		Dist<bool>::MPI_DCCols A, AT;	// construct object
		AT.ReadDistribute(ifilename, 0);	// read it from file, note that we use the transpose of "input" data
		A = AT;
		A.Transpose();
			
		int nPasses = (int) pow(2.0, K4Approx);
		int numBatches = (int) ceil( static_cast<float>(nPasses)/ static_cast<float>(batchSize));
	
		// get the number of batch vertices for submatrix
		int subBatchSize = batchSize / (AT.getcommgrid())->GetGridCols();
		int nBatchSize = subBatchSize * (AT.getcommgrid())->GetGridCols();
		nPasses = numBatches * nBatchSize;	// update the number of starting vertices	
	
		if(batchSize % (AT.getcommgrid())->GetGridCols() > 0 && myrank == 0)
		{
			cout << "*** Batchsize is not evenly divisible by the grid dimension ***" << endl;
			cout << "*** Processing "<< nPasses <<" vertices instead"<< endl;
		}

		vector<int> candidates;
		if (myrank == 0)
			cout << "Batch processing will occur " << numBatches << " times, each processing " << nBatchSize << " vertices (overall)" << endl;

		// Only consider non-isolated vertices
		int vertices = 0;
		int vrtxid = 0; 
		int nlocpass = numBatches * subBatchSize;
		while(vertices < nlocpass)
		{
			vector<int> single;
			vector<int> empty;
			single.push_back(vrtxid);		// will return ERROR if vrtxid > N (the column dimension) 
			int locnnz = ((AT.seq())(empty,single)).getnnz();
			int totnnz;
			MPI_Allreduce( &locnnz, &totnnz, 1, MPI_INT, MPI_SUM, (AT.getcommgrid())->GetColWorld());
					
			if(totnnz > 0)
			{
				candidates.push_back(vrtxid);
				++vertices;
			}
			++vrtxid;
		}
		
		SpParHelper::Print("Candidates chosen, precomputation finished\n");
		double t1 = MPI_Wtime();
		vector<int> batch(subBatchSize);
		DenseParVec<int, double> bc(AT.getcommgrid(),0.0);

		for(int i=0; i< numBatches; ++i)
		{
			for(int j=0; j< subBatchSize; ++j)
			{
				batch[j] = candidates[i*subBatchSize + j];
			}
			
			Dist<int>::MPI_DCCols fringe = AT.SubsRefCol(batch);

			// Create nsp by setting (r,i)=1 for the ith root vertex with label r
			// Inially only the diagonal processors have any nonzeros (because we chose roots so)
			Dist<int>::DCCols * nsploc = new Dist<int>::DCCols();
			tuple<int, int, int> * mytuples = NULL;	
			if(AT.getcommgrid()->GetRankInProcRow() == AT.getcommgrid()->GetRankInProcCol())
			{
				mytuples = new tuple<int, int, int>[subBatchSize];
				for(int k =0; k<subBatchSize; ++k)
				{
					mytuples[k] = make_tuple(batch[k], k, 1);
				}
				nsploc->Create( subBatchSize, AT.getlocalrows(), subBatchSize, mytuples);		
			}
			else
			{  
				nsploc->Create( 0, AT.getlocalrows(), subBatchSize, mytuples);		
			}
		
			Dist<int>::MPI_DCCols  nsp(nsploc, AT.getcommgrid());			
			vector < Dist<bool>::MPI_DCCols * > bfs;		// internally keeps track of depth

			SpParHelper::Print("Exploring via BFS...\n");
			while( fringe.getnnz() > 0 )
			{
				nsp += fringe;
				Dist<bool>::MPI_DCCols * level = new Dist<bool>::MPI_DCCols( fringe ); 
				bfs.push_back(level);

				fringe = PSpGEMM<PTBOOLINT>(AT, fringe);
				fringe = EWiseMult(fringe, nsp, true);
			}

			// Apply the unary function 1/x to every element in the matrix
			// 1/x works because no explicit zeros are stored in the sparse matrix nsp
			Dist<double>::MPI_DCCols nspInv = nsp;
			nspInv.Apply(bind1st(divides<double>(), 1));

			// create a dense matrix with all 1's 
			DenseParMat<int, double> bcu(1.0, AT.getcommgrid(), fringe.getlocalrows(), fringe.getlocalcols() );

			SpParHelper::Print("Tallying...\n");
			// BC update for all vertices except the sources
			for(int j = bfs.size()-1; j > 0; --j)
			{
				Dist<double>::MPI_DCCols w = EWiseMult( *bfs[j], nspInv, false);
				w.EWiseScale(bcu);

				Dist<double>::MPI_DCCols product = PSpGEMM<PTBOOLDOUBLE>(A,w);
				product = EWiseMult(product, *bfs[j-1], false);
				product = EWiseMult(product, nsp, false);		

				bcu += product;
			}
			for(int j=0; j < bfs.size(); ++j)
			{
				delete bfs[j];
			}
		
			SpParHelper::Print("Adding bc contributions...\n");			
			bc += bcu.Reduce(Row, plus<double>(), 0.0);	// pack along rows
		}
		bc.Apply(bind2nd(minus<double>(), nPasses));	// Subtrack nPasses from all the bc scores (because bcu was initialized to all 1's)
		
		double t2=MPI_Wtime();
		double TEPS = (nPasses * static_cast<float>(A.getnnz())) / (t2-t1);
		if( myrank == 0)
		{
			cout<<"Computation finished"<<endl;	
			fprintf(stdout, "%.6lf seconds elapsed for %d starting vertices\n", t2-t1, nPasses);
			fprintf(stdout, "TEPS score is: %.6lf\n", TEPS);
		}
	}	

	// make sure the destructors for all objects are called before MPI::Finalize()
	MPI_Finalize();
	return 0;
}

