/****************************************************************/
/* Parallel Combinatorial BLAS Library (for Graph Computations) */
/* version 1.2 -------------------------------------------------*/
/* date: 10/06/2011 --------------------------------------------*/
/* authors: Aydin Buluc (abuluc@lbl.gov), Adam Lugowski --------*/
/****************************************************************/
/*
Copyright (c) 2011, Aydin Buluc

WARNING: This file is deprecated with Combinatorial v1.2
Please consider using the FullyDistVec object

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

#include "DenseParVec.h"
#include "SpParVec.h"
#include "Operations.h"

template <class IT, class NT>
DenseParVec<IT, NT>::DenseParVec ()
{
	zero = static_cast<NT>(0);
	commGrid.reset(new CommGrid(MPI_COMM_WORLD, 0, 0));

	if(commGrid->GetRankInProcRow() == commGrid->GetRankInProcCol())
		diagonal = true;
	else
		diagonal = false;	
}

// Create a new distributed dense array with all values initialized to zero
template<class IT, class NT> 
DenseParVec<IT, NT>::DenseParVec (IT globallength)
{
	zero = static_cast<NT>(0);
	commGrid.reset(new CommGrid(MPI_COMM_WORLD, 0, 0));
	if(commGrid->GetRankInProcRow() == commGrid->GetRankInProcCol())
		diagonal = true;
	else
		diagonal = false;	
	
	if (diagonal)
	{
		int nprocs = commGrid->GetDiagSize();
		int ndrank = commGrid->GetDiagRank();

		IT typical = globallength/nprocs;
		if(ndrank == nprocs - 1)
			arr.resize(globallength - ndrank*typical, zero);
		else
			arr.resize(typical, zero);
	}
}

template <class IT, class NT>
DenseParVec<IT, NT>::DenseParVec (IT locallength, NT initval, NT id): zero(id)
{
	commGrid.reset(new CommGrid(MPI_COMM_WORLD, 0, 0));

	if(commGrid->GetRankInProcRow() == commGrid->GetRankInProcCol())
		diagonal = true;
	else
		diagonal = false;
		
	if (diagonal)
		arr.resize(locallength, initval);
}

template <class IT, class NT>
DenseParVec<IT, NT>::DenseParVec ( shared_ptr<CommGrid> grid, NT id): zero(id)
{
	commGrid.reset(new CommGrid(*grid));		
	if(commGrid->GetRankInProcRow() == commGrid->GetRankInProcCol())
		diagonal = true;
	else
		diagonal = false;	
};

template <class IT, class NT>
DenseParVec<IT, NT>::DenseParVec ( shared_ptr<CommGrid> grid, IT locallength, NT initval, NT id): commGrid(grid), zero(id)
{
	if(commGrid->GetRankInProcRow() == commGrid->GetRankInProcCol())
		diagonal = true;
	else
		diagonal = false;	

	if (diagonal)
		arr.resize(locallength, initval);
};

template <class IT, class NT>
template <typename _BinaryOperation>
NT DenseParVec<IT,NT>::Reduce(_BinaryOperation __binary_op, NT identity)
{
	// std::accumulate returns identity for empty sequences
	NT localsum = std::accumulate( arr.begin(), arr.end(), identity, __binary_op);

	NT totalsum = identity;
	MPI_Allreduce( &localsum, &totalsum, 1, MPIType<NT>(), MPIOp<_BinaryOperation, NT>::op(), commGrid->GetWorld());
	return totalsum;
}

template <class IT, class NT>
DenseParVec< IT,NT > &  DenseParVec<IT,NT>::operator=(const SpParVec< IT,NT > & rhs)		// SpParVec->DenseParVec conversion operator
{
	arr.resize(rhs.length);
	std::fill(arr.begin(), arr.end(), zero);	

	IT spvecsize = rhs.ind.size();
	for(IT i=0; i< spvecsize; ++i)
	{
		arr[rhs.ind[i]] = rhs.num[i];
	}
	
	return *this;
}

template <class IT, class NT>
DenseParVec< IT,NT > &  DenseParVec<IT,NT>::operator=(const DenseParVec< IT,NT > & rhs)	
{
	if (this == &rhs)      // Same object?
      		return *this;        // Yes, so skip assignment, and just return *this.
	commGrid.reset(new CommGrid(*(rhs.commGrid)));		
	arr = rhs.arr;
	diagonal = rhs.diagonal;
	zero = rhs.zero;
	return *this;
}

template <class IT, class NT>
DenseParVec< IT,NT > &  DenseParVec<IT,NT>::stealFrom(DenseParVec<IT,NT> & victim)		// SpParVec->DenseParVec conversion operator
{
	commGrid.reset(new CommGrid(*(victim.commGrid)));		
	arr.swap(victim.arr);
	diagonal = victim.diagonal;
	zero = victim.zero;
	
	return *this;
}

template <class IT, class NT>
DenseParVec< IT,NT > &  DenseParVec<IT,NT>::operator+=(const SpParVec< IT,NT > & rhs)		
{
	IT spvecsize = rhs.ind.size();
	for(IT i=0; i< spvecsize; ++i)
	{
		if(arr[rhs.ind[i]] == zero) // not set before
			arr[rhs.ind[i]] = rhs.num[i];
		else
			arr[rhs.ind[i]] += rhs.num[i];
	}
	return *this;
}

template <class IT, class NT>
DenseParVec< IT,NT > &  DenseParVec<IT,NT>::operator-=(const SpParVec< IT,NT > & rhs)		
{
	IT spvecsize = rhs.ind.size();
	for(IT i=0; i< spvecsize; ++i)
	{
		arr[rhs.ind[i]] -= rhs.num[i];
	}
}


/**
  * Perform __binary_op(*this, v2) for every element in rhs, *this, 
  * which are of the same size. and write the result back to *this
  */ 
template <class IT, class NT>
template <typename _BinaryOperation>	
void DenseParVec<IT,NT>::EWise(const DenseParVec<IT,NT> & rhs,  _BinaryOperation __binary_op)
{
	if(zero == rhs.zero)
	{
		transform ( arr.begin(), arr.end(), rhs.arr.begin(), arr.begin(), __binary_op );
	}
	else
	{
		cout << "DenseParVec objects have different identity (zero) elements..." << endl;
		cout << "Operation didn't happen !" << endl;
	}
};


template <class IT, class NT>
DenseParVec<IT,NT> & DenseParVec<IT, NT>::operator+=(const DenseParVec<IT,NT> & rhs)
{
	if(this != &rhs)		
	{	
		if(!(*commGrid == *rhs.commGrid)) 		
		{
			cout << "Grids are not comparable elementwise addition" << endl; 
			MPI_Abort(MPI_COMM_WORLD,GRIDMISMATCH);
		}
		else if(diagonal)	// Only the diagonal processors hold values
		{
			EWise(rhs, std::plus<NT>());
		} 	
	}	
	return *this;
};

template <class IT, class NT>
DenseParVec<IT,NT> & DenseParVec<IT, NT>::operator-=(const DenseParVec<IT,NT> & rhs)
{
	if(this != &rhs)		
	{	
		if(!(*commGrid == *rhs.commGrid)) 		
		{
			cout << "Grids are not comparable elementwise addition" << endl; 
			MPI_Abort(MPI_COMM_WORLD,GRIDMISMATCH);
		}
		else if(diagonal)	// Only the diagonal processors hold values
		{
			EWise(rhs, std::minus<NT>());
		} 	
	}	
	return *this;
};		

template <class IT, class NT>
bool DenseParVec<IT,NT>::operator==(const DenseParVec<IT,NT> & rhs) const
{
	ErrorTolerantEqual<NT> epsilonequal(EPSILON);
	int local = 1;
	if(diagonal)
	{
		local = (int) std::equal(arr.begin(), arr.end(), rhs.arr.begin(), epsilonequal );
#ifdef DEBUG
		vector<NT> diff(arr.size());
		transform(arr.begin(), arr.end(), rhs.arr.begin(), diff.begin(), minus<NT>());
		typename vector<NT>::iterator maxitr;
		maxitr = max_element(diff.begin(), diff.end()); 			
		cout << maxitr-diff.begin() << ": " << *maxitr << " where lhs: " << *(arr.begin()+(maxitr-diff.begin())) 
						<< " and rhs: " << *(rhs.arr.begin()+(maxitr-diff.begin())) << endl; 
#endif
	}
	int whole = 1;
	MPI_Allreduce( &local, &whole, 1, MPI_INT, MPI_BAND, commGrid->GetWorld());
	return static_cast<bool>(whole);	
}

template <class IT, class NT>
template <typename _Predicate>
IT DenseParVec<IT,NT>::Count(_Predicate pred) const
{
	IT local = 0;
	if(diagonal)
	{
		local = count_if( arr.begin(), arr.end(), pred );
	}
	IT whole = 0;
	MPI_Allreduce( &local, &whole, 1, MPIType<IT>(), MPI_SUM, commGrid->GetWorld());
	return whole;	
}


//! Returns a dense vector of global indices 
//! for which the predicate is satisfied
template <class IT, class NT>
template <typename _Predicate>
DenseParVec<IT,IT> DenseParVec<IT,NT>::FindInds(_Predicate pred) const
{
	DenseParVec<IT,IT> found(commGrid, (IT) 0);
	MPI_Comm DiagWorld = commGrid->GetDiagWorld();
	if(DiagWorld != MPI_COMM_NULL) // Diagonal processors only
	{
	        int dgrank, nprocs;
		MPI_Comm_rank(DiagWorld, &dgrank);
		MPI_Comm_size(DiagWorld, &nprocs);
		IT old_n_perproc = getTypicalLocLength();
		
		IT size = arr.size();
		for(IT i=0; i<size; ++i)
		{
			if(pred(arr[i]))
			{
				found.arr.push_back(i+old_n_perproc*dgrank);
			}
		}
		MPI_Barrier(DiagWorld);
		
		// Since the found vector is not reshuffled yet, we can't use getTypicalLocLength() at this point
		IT n_perproc = found.getTotalLength(DiagWorld) / nprocs;
		if(n_perproc == 0)	// it has less than sqrt(p) elements, all owned by the last processor
		{
			if(dgrank != nprocs-1)
			{
				int arrsize = found.arr.size();
				MPI_Gather(&arrsize, 1, MPI_INT, NULL, 1, MPI_INT, nprocs-1, DiagWorld);
				MPI_Gatherv(&(found.arr[0]), arrsize, MPIType<IT>(), NULL, NULL, NULL, MPIType<IT>(), nprocs-1, DiagWorld);
			}
			else
			{	
				int * allnnzs = new int[nprocs];
				allnnzs[dgrank] = found.arr.size();
				MPI_Gather(MPI_IN_PLACE, 1, MPI_INT, allnnzs, 1, MPI_INT, nprocs-1, DiagWorld);
				
				int * rdispls = new int[nprocs];
				rdispls[0] = 0;
				for(int i=0; i<nprocs-1; ++i)
					rdispls[i+1] = rdispls[i] + allnnzs[i];

				IT totrecv = accumulate(allnnzs, allnnzs+nprocs, 0);
				vector<IT> recvbuf(totrecv);
				MPI_Gatherv(MPI_IN_PLACE, 1, MPI_INT, &(recvbuf[0]), allnnzs, rdispls, MPIType<IT>(), nprocs-1, DiagWorld);

				found.arr.swap(recvbuf);
				DeleteAll(allnnzs, rdispls);
			}
			return found;		// don't execute further
		}
		IT lengthuntil = dgrank * n_perproc;

		// rebalance/redistribute
		IT nsize = found.arr.size();
		int * sendcnt = new int[nprocs];
		fill(sendcnt, sendcnt+nprocs, 0);
		for(IT i=0; i<nsize; ++i)
		{
			// owner id's are monotonically increasing and continuous
			int owner = std::min(static_cast<int>( (i+lengthuntil) / n_perproc), nprocs-1); 
			sendcnt[owner]++;
		}

		int * recvcnt = new int[nprocs];
		MPI_Alltoall(sendcnt, 1, MPI_INT, recvcnt, 1, MPI_INT, DiagWorld); // share the counts

		int * sdispls = new int[nprocs];
		int * rdispls = new int[nprocs];
		sdispls[0] = 0;
		rdispls[0] = 0;
		for(int i=0; i<nprocs-1; ++i)
		{
			sdispls[i+1] = sdispls[i] + sendcnt[i];
			rdispls[i+1] = rdispls[i] + recvcnt[i];
		}

		IT totrecv = accumulate(recvcnt,recvcnt+nprocs, (IT) 0);
		vector<IT> recvbuf(totrecv);
			
		// data is already in the right order in found.arr
		MPI_Alltoallv(&(found.arr[0]), sendcnt, sdispls, MPIType<IT>(), &(recvbuf[0]), recvcnt, rdispls, MPIType<IT>(), DiagWorld);
		found.arr.swap(recvbuf);
		DeleteAll(sendcnt, recvcnt, sdispls, rdispls);
	}
	return found;
}



//! Requires no communication because SpParVec (the return object)
//! is distributed based on length, not nonzero counts
template <class IT, class NT>
template <typename _Predicate>
SpParVec<IT,NT> DenseParVec<IT,NT>::Find(_Predicate pred) const
{
	SpParVec<IT,NT> found(commGrid);
	if(diagonal)
	{
		IT size = arr.size();
		for(IT i=0; i<size; ++i)
		{
			if(pred(arr[i]))
			{
				found.ind.push_back(i);
				found.num.push_back(arr[i]);
			}
		}
		found.length = size;
	}
	return found;	
}

template <class IT, class NT>
ifstream& DenseParVec<IT,NT>::ReadDistribute (ifstream& infile, int master)
{
	SpParVec<IT,NT> tmpSpVec(commGrid);
	tmpSpVec.ReadDistribute(infile, master);

	*this = tmpSpVec;
	return infile;
}

template <class IT, class NT>
void DenseParVec<IT,NT>::SetElement (IT indx, NT numx)
{
	MPI_Comm DiagWorld = commGrid->GetDiagWorld();
	if(DiagWorld != MPI_COMM_NULL) // Diagonal processors only
	{
	        int dgrank, nprocs;
		MPI_Comm_rank(DiagWorld, &dgrank);
		MPI_Comm_size(DiagWorld, &nprocs);
		IT n_perproc = getTypicalLocLength();	
		IT offset = dgrank * n_perproc;
		
		if (n_perproc == 0) {
			cout << "DenseParVec::SetElement can't be called on an empty vector." << endl;
			return;
		}
		IT owner = std::min(static_cast<int>(indx / n_perproc), nprocs-1);	
		if(owner == dgrank) // this process is the owner
		{
			IT locindx = indx-offset;
			
			if (locindx > arr.size()-1)
			{
				cout << "DenseParVec::SetElement cannot expand array" << endl;
			}
			else if (locindx < 0)
			{
				cout << "DenseParVec::SetElement local index < 0" << endl;
			}
			else
			{
				arr[locindx] = numx;
			}
		}
	}
}

template <class IT, class NT>
NT DenseParVec<IT,NT>::GetElement (IT indx) const
{
	NT ret;
	int owner = 0;
	MPI_Comm DiagWorld = commGrid->GetDiagWorld();
	if(DiagWorld != MPI_COMM_NULL) // Diagonal processors only
	{
	        int dgrank, nprocs;
		MPI_Comm_rank(DiagWorld, &dgrank);
		MPI_Comm_size(DiagWorld, &nprocs);
		IT n_perproc = getTypicalLocLength();
		IT offset = dgrank * n_perproc;
		
		if (n_perproc == 0 && dgrank == 0) {
			cout << "DenseParVec::GetElement can't be called on an empty vector." << endl;
			return numeric_limits<NT>::min();
		}
		owner = std::min(static_cast<int>(indx / n_perproc), nprocs-1);	
		if(owner == dgrank) // this process is the owner
		{
			IT locindx = indx-offset;
			if (locindx > arr.size()-1)
			{
				cout << "DenseParVec::GetElement cannot expand array" << endl;
			}
			else if (locindx < 0)
			{
				cout << "DenseParVec::GetElement local index < 0" << endl;
			}
			else
			{
				ret = arr[locindx];
			}
		}
	}
	int worldowner = commGrid->GetRank(owner);	// 0 is always on the diagonal
	MPI_Bcast(&worldowner, 1, MPIType<int>(), 0, commGrid->GetWorld());
	MPI_Bcast(&ret, 1, MPIType<NT>(), worldowner, commGrid->GetWorld());
	return ret;
}

template <class IT, class NT>
void DenseParVec<IT,NT>::DebugPrint()
{
	// ABAB: Alternative
	// ofstream out;
	// commGrid->OpenDebugFile("DenseParVec", out);
	// copy(recvbuf, recvbuf+totrecv, ostream_iterator<IT>(out, " "));
	// out << " <end_of_vector>"<< endl;

	MPI_Comm DiagWorld = commGrid->GetDiagWorld();
	if(DiagWorld != MPI_COMM_NULL) // Diagonal processors only
	{
	        int dgrank, nprocs;
		MPI_Comm_rank(DiagWorld, &dgrank);
		MPI_Comm_size(DiagWorld, &nprocs);

		int64_t* all_nnzs = new int64_t[nprocs];
		
		all_nnzs[dgrank] = arr.size();
		MPI_Allgather(MPI_IN_PLACE, 1, MPIType<int64_t>(), all_nnzs, 1, MPIType<int64_t>(), DiagWorld);
		int64_t offset = 0;
		
		for (int i = 0; i < nprocs; i++)
		{
			if (i == dgrank)
			{
				cerr << arr.size() << " elements stored on proc " << dgrank << "," << dgrank << ":" ;
				
				for (int j = 0; j < arr.size(); j++)
				{
					cerr << "\n[" << (j+offset) << "] = " << arr[j] ;
				}
				cerr << endl;
			}
			offset += all_nnzs[i];
			MPI_Barrier(DiagWorld);
		}
		MPI_Barrier(DiagWorld);
		if (dgrank == 0)
			cerr << "total size: " << offset << endl;
		MPI_Barrier(DiagWorld);
	}
}

template <class IT, class NT>
template <typename _UnaryOperation>
void DenseParVec<IT,NT>::Apply(_UnaryOperation __unary_op, const SpParVec<IT,NT> & mask)
{
	typename vector< IT >::const_iterator miter = mask.ind.begin();
	while (miter < mask.ind.end())
	{
		IT index = *miter++;
		arr[index] = __unary_op(arr[index]);
	}
}	

// Randomly permutes an already existing vector
template <class IT, class NT>
void DenseParVec<IT,NT>::RandPerm()
{
	MPI_Comm DiagWorld = commGrid->GetDiagWorld();
	if(DiagWorld != MPI_COMM_NULL) // Diagonal processors only
	{
		IT size = arr.size();
		pair<double,IT> * vecpair = new pair<double,IT>[size];

	        int dgrank, nprocs;
		MPI_Comm_rank(DiagWorld, &dgrank);
		MPI_Comm_size(DiagWorld, &nprocs);

		long * dist = new long[nprocs];
		dist[dgrank] = size;
		MPI_Allgather(MPI_IN_PLACE, 1, MPIType<long>(), dist, 1, MPIType<long>(), DiagWorld);
		IT lengthuntil = accumulate(dist, dist+dgrank, 0);

  		MTRand M;	// generate random numbers with Mersenne Twister
		for(int i=0; i<size; ++i)
		{
			vecpair[i].first = M.rand();
			vecpair[i].second = arr[i];
		}

		// less< pair<T1,T2> > works correctly (sorts wrt first elements)	
    		vpsort::parallel_sort (vecpair, vecpair + size,  dist, DiagWorld);

		vector< NT > nnum(size);
		for(int i=0; i<size; ++i)
			nnum[i] = vecpair[i].second;

		delete [] vecpair;
		delete [] dist;

		arr.swap(nnum);
	}
}

template <class IT, class NT>
void DenseParVec<IT,NT>::iota(IT size, NT first)
{
	MPI_Comm DiagWorld = commGrid->GetDiagWorld();
	if(DiagWorld != MPI_COMM_NULL) // Diagonal processors only
	{
	        int dgrank, nprocs;
		MPI_Comm_rank(DiagWorld, &dgrank);
		MPI_Comm_size(DiagWorld, &nprocs);
		IT n_perproc = size / nprocs;

		IT length = (dgrank != nprocs-1) ? n_perproc: (size - (n_perproc * (nprocs-1)));
		arr.resize(length);
		SpHelper::iota(arr.begin(), arr.end(), (dgrank * n_perproc) + first);	// global across processors
	}
}

template <class IT, class NT>
DenseParVec<IT,NT> DenseParVec<IT,NT>::operator() (const DenseParVec<IT,IT> & ri) const
{
	if(!(*commGrid == *ri.commGrid))
	{
		cout << "Grids are not comparable for dense vector subsref" << endl;
		return DenseParVec<IT,NT>();
	}

	MPI_Comm DiagWorld = commGrid->GetDiagWorld();
	DenseParVec<IT,NT> Indexed(commGrid, zero);	// length(Indexed) = length(ri)
	if(DiagWorld != MPI_COMM_NULL) 		// Diagonal processors only
	{
	        int dgrank, nprocs;
		MPI_Comm_rank(DiagWorld, &dgrank);
		MPI_Comm_size(DiagWorld, &nprocs);
		IT n_perproc = getTypicalLocLength();
		vector< vector< IT > > data_req(nprocs);	
		vector< vector< IT > > revr_map(nprocs);	// to put the incoming data to the correct location	
		for(IT i=0; i < ri.arr.size(); ++i)
		{
			int owner = ri.arr[i] / n_perproc;	// numerical values in ri are 0-based
			owner = std::min(owner, nprocs-1);	// find its owner 
			data_req[owner].push_back(ri.arr[i] - (n_perproc * owner));
			revr_map[owner].push_back(i);
		}
		IT * sendbuf = new IT[ri.arr.size()];
		int * sendcnt = new int[nprocs];
		int * sdispls = new int[nprocs];
		for(int i=0; i<nprocs; ++i)
			sendcnt[i] = data_req[i].size();

		int * rdispls = new int[nprocs];
		int * recvcnt = new int[nprocs];
		MPI_Alltoall(sendcnt, 1, MPI_INT, recvcnt, 1, MPI_INT, DiagWorld);      // share the request count

		sdispls[0] = 0;
		rdispls[0] = 0;
		for(int i=0; i<nprocs-1; ++i)
		{
			sdispls[i+1] = sdispls[i] + sendcnt[i];
			rdispls[i+1] = rdispls[i] + recvcnt[i];
		}
		IT totrecv = accumulate(recvcnt,recvcnt+nprocs,zero);
		IT * recvbuf = new IT[totrecv];

		for(int i=0; i<nprocs; ++i)
		{
			copy(data_req[i].begin(), data_req[i].end(), sendbuf+sdispls[i]);
			vector<IT>().swap(data_req[i]);
		}

		IT * reversemap = new IT[ri.arr.size()];
		for(int i=0; i<nprocs; ++i)
		{
			copy(revr_map[i].begin(), revr_map[i].end(), reversemap+sdispls[i]);
			vector<IT>().swap(revr_map[i]);
		}
		MPI_Alltoallv(sendbuf, sendcnt, sdispls, MPIType<IT>(), recvbuf, recvcnt, rdispls, MPIType<IT>(), DiagWorld); // request data

		// We will return the requested data,
		// our return will be as big as the request 
		// as we are indexing a dense vector, all elements exist
		// so the displacement boundaries are the same as rdispls
		NT * databack = new NT[totrecv];		

		for(int i=0; i<nprocs; ++i)
		{
			for(int j = rdispls[i]; j < rdispls[i] + recvcnt[i]; ++j)	// fetch the numerical values
			{
				databack[j] = arr[recvbuf[j]];
			}
		}
		
		delete [] recvbuf;
		NT * databuf = new NT[ri.arr.size()];

		// the response counts are the same as the request counts 
		MPI_Alltoallv(databack, recvcnt, rdispls, MPIType<NT>(), databuf, sendcnt, sdispls, MPIType<NT>(), DiagWorld);  // send data
		DeleteAll(rdispls, recvcnt, databack);

		// Now create the output from databuf
		Indexed.arr.resize(ri.arr.size()); 
		for(int i=0; i<nprocs; ++i)
		{
			for(int j=sdispls[i]; j< sdispls[i]+sendcnt[i]; ++j)
			{
				Indexed.arr[reversemap[j]] = databuf[j];
			}
		}
		DeleteAll(sdispls, sendcnt, databuf,reversemap);
	}
	return Indexed;
}

template <class IT, class NT>
IT DenseParVec<IT,NT>::getTotalLength(MPI_Comm & comm) const
{
	IT totnnz = 0;
	if (comm != MPI_COMM_NULL)	
	{
		IT locnnz = arr.size();
		MPI_Allreduce( &locnnz, & totnnz, 1, MPIType<IT>(), MPI_SUM, comm);
	}
	return totnnz;
}

template <class IT, class NT>
IT DenseParVec<IT,NT>::getTypicalLocLength() const
{
	IT n_perproc = 0 ;
	MPI_Comm DiagWorld = commGrid->GetDiagWorld();
        if(DiagWorld != MPI_COMM_NULL) // Diagonal processors only
        {
	        int dgrank, nprocs;
		MPI_Comm_rank(DiagWorld, &dgrank);
		MPI_Comm_size(DiagWorld, &nprocs);
                n_perproc = arr.size(); 
                if (dgrank == nprocs-1 && nprocs > 1)
                {
                        // the local length on the last processor will be greater than the others if the vector length is not evenly divisible
                        // but for these calculations we need that length
			MPI_Recv(&n_perproc, 1, MPIType<IT>(), 0, 1, DiagWorld, NULL);
                }
                else if (dgrank == 0 && nprocs > 1)
                {
			MPI_Send(&n_perproc, 1, MPIType<IT>(), nprocs-1, 1, DiagWorld);
                }
	}
	return n_perproc;
}



template <class IT, class NT>
void DenseParVec<IT,NT>::PrintInfo(string vectorname) const
{
	IT totl = getTotalLength(commGrid->GetDiagWorld());
	if (commGrid->GetRank() == 0)		// is always on the diagonal
		cout << "As a whole, " << vectorname << " has length " << totl << endl; 
}
