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

#include "FullyDistVec.h"
#include "FullyDistSpVec.h"
#include "Operations.h"

template <class IT, class NT>
FullyDistVec<IT, NT>::FullyDistVec ()
: FullyDist<IT,NT,typename CombBLAS::disable_if< CombBLAS::is_boolean<NT>::value, NT >::type>()
{ 
}

template <class IT, class NT>
FullyDistVec<IT, NT>::FullyDistVec (IT globallen, NT initval)
:FullyDist<IT,NT,typename CombBLAS::disable_if< CombBLAS::is_boolean<NT>::value, NT >::type>(globallen)
{
	arr.resize(MyLocLength(), initval);
}

template <class IT, class NT>
FullyDistVec<IT, NT>::FullyDistVec ( shared_ptr<CommGrid> grid)
: FullyDist<IT,NT,typename CombBLAS::disable_if< CombBLAS::is_boolean<NT>::value, NT >::type>(grid)
{ }

template <class IT, class NT>
FullyDistVec<IT, NT>::FullyDistVec ( shared_ptr<CommGrid> grid, IT globallen, NT initval)
: FullyDist<IT,NT,typename CombBLAS::disable_if< CombBLAS::is_boolean<NT>::value, NT >::type>(grid,globallen)
{
	arr.resize(MyLocLength(), initval);
}

template <class IT, class NT>
FullyDistVec<IT,NT>::FullyDistVec (const FullyDistSpVec<IT,NT> & rhs)		// Conversion copy-constructor
{
	*this = rhs;
}

template <class IT, class NT>
template <class ITRHS, class NTRHS>
FullyDistVec<IT, NT>::FullyDistVec ( const FullyDistVec<ITRHS, NTRHS>& rhs )
: FullyDist<IT,NT,typename CombBLAS::disable_if< CombBLAS::is_boolean<NT>::value, NT >::type>(rhs.commGrid, static_cast<IT>(rhs.glen))
{
	arr.resize(static_cast<IT>(rhs.arr.size()), NT());
	
	for(IT i=0; (unsigned)i < arr.size(); ++i)
	{
		arr[i] = static_cast<NT>(rhs.arr[static_cast<ITRHS>(i)]);
	}
}

template <class IT, class NT>
template <typename _BinaryOperation>
NT FullyDistVec<IT,NT>::Reduce(_BinaryOperation __binary_op, NT identity)
{
	// std::accumulate returns identity for empty sequences
	NT localsum = std::accumulate( arr.begin(), arr.end(), identity, __binary_op);

	NT totalsum = identity;
	MPI_Allreduce( &localsum, &totalsum, 1, MPIType<NT>(), MPIOp<_BinaryOperation, NT>::op(), commGrid->GetWorld());
	return totalsum;
}

template <class IT, class NT>
template <typename OUT, typename _BinaryOperation, typename _UnaryOperation>
OUT FullyDistVec<IT,NT>::Reduce(_BinaryOperation __binary_op, OUT default_val, _UnaryOperation __unary_op)
{
	// std::accumulate returns identity for empty sequences
	OUT localsum = default_val; 
	
	if (arr.size() > 0)
	{
		typename vector< NT >::const_iterator iter = arr.begin();
		//localsum = __unary_op(*iter);
		//iter++;
		while (iter < arr.end())
		{
			localsum = __binary_op(localsum, __unary_op(*iter));
			iter++;
		}
	}

	OUT totalsum = default_val;
	MPI_Allreduce( &localsum, &totalsum, 1, MPIType<OUT>(), MPIOp<_BinaryOperation, OUT>::op(), commGrid->GetWorld());
	return totalsum;
}


//! ABAB: Put concept check, NT should be integer for this to make sense
template<class IT, class NT>
void FullyDistVec<IT,NT>::SelectCandidates(double nver, bool deterministic)
{
	MTRand M;       // generate random numbers with Mersenne Twister
	if(deterministic)
		M.seed(1);

	IT length = TotalLength();
	vector<double> loccands(length);
	vector<NT> loccandints(length);
	MPI_Comm World = commGrid->GetWorld();
	int myrank = commGrid->GetRank();
	if(myrank == 0)
	{
		for(int i=0; i<length; ++i)
			loccands[i] = M.rand();
		transform(loccands.begin(), loccands.end(), loccands.begin(), bind2nd( multiplies<double>(), nver ));
		
		for(int i=0; i<length; ++i)
			loccandints[i] = static_cast<NT>(loccands[i]);
	}
	MPI_Bcast(&(loccandints[0]), length, MPIType<NT>(),0, World);
	for(IT i=0; i<length; ++i)
		SetElement(i,loccandints[i]);
}

template <class IT, class NT>
template <class ITRHS, class NTRHS>
FullyDistVec< IT,NT > &  FullyDistVec<IT,NT>::operator=(const FullyDistVec< ITRHS,NTRHS > & rhs)	
{
	if(static_cast<const void*>(this) != static_cast<const void*>(&rhs))		
	{
		//FullyDist<IT,NT>::operator= (rhs);	// to update glen and commGrid
		glen = static_cast<IT>(rhs.glen);
		commGrid = rhs.commGrid;
		
		arr.resize(rhs.arr.size(), NT());
		for(IT i=0; (unsigned)i < arr.size(); ++i)
		{
			arr[i] = static_cast<NT>(rhs.arr[static_cast<ITRHS>(i)]);
		}
	}
	return *this;
}	

template <class IT, class NT>
FullyDistVec< IT,NT > &  FullyDistVec<IT,NT>::operator=(const FullyDistVec< IT,NT > & rhs)	
{
	if(this != &rhs)		
	{
		FullyDist<IT,NT,typename CombBLAS::disable_if< CombBLAS::is_boolean<NT>::value, NT >::type>::operator= (rhs);	// to update glen and commGrid
		arr = rhs.arr;
	}
	return *this;
}	

template <class IT, class NT>
FullyDistVec< IT,NT > &  FullyDistVec<IT,NT>::operator=(const FullyDistSpVec< IT,NT > & rhs)		// FullyDistSpVec->FullyDistVec conversion operator
{
	FullyDist<IT,NT,typename CombBLAS::disable_if< CombBLAS::is_boolean<NT>::value, NT >::type>::operator= (rhs);	// to update glen and commGrid
	arr.resize(rhs.MyLocLength());
	std::fill(arr.begin(), arr.end(), NT());	

	IT spvecsize = rhs.getlocnnz();
	for(IT i=0; i< spvecsize; ++i)
	{
		//if(rhs.ind[i] > arr.size())
		//	cout << "rhs.ind[i]: " << rhs.ind[i] <<  endl;
		arr[rhs.ind[i]] = rhs.num[i];
	}
	return *this;
}

template <class IT, class NT>
FullyDistVec< IT,NT > &  FullyDistVec<IT,NT>::operator=(const DenseParVec< IT,NT > & rhs)		// DenseParVec->FullyDistVec conversion operator
{
	if(*commGrid != *rhs.commGrid) 		
	{
		cout << "Grids are not comparable elementwise addition" << endl; 
		MPI_Abort(MPI_COMM_WORLD, GRIDMISMATCH);
	}
	else
	{
		glen = rhs.getTotalLength();
		arr.resize(MyLocLength());	// once glen is set, MyLocLength() works
		fill(arr.begin(), arr.end(), NT());	

		int * sendcnts = NULL;
		int * dpls = NULL;
		if(rhs.diagonal)
		{
			int proccols = commGrid->GetGridCols();	
        		IT n_perproc = rhs.getLocalLength() / proccols;
			sendcnts = new int[proccols];
			fill(sendcnts, sendcnts+proccols-1, n_perproc);
			sendcnts[proccols-1] = rhs.getLocalLength() - (n_perproc * (proccols-1));
			dpls = new int[proccols]();	// displacements (zero initialized pid) 
			partial_sum(sendcnts, sendcnts+proccols-1, dpls+1);
		}

		int rowroot = commGrid->GetDiagOfProcRow();
		MPI_Scatterv(&(rhs.arr[0]),sendcnts, dpls, MPIType<NT>(), &(arr[0]), arr.size(), MPIType<NT>(),rowroot, commGrid->GetRowWorld());
	}
	return *this;
}


// Let the compiler create an assignment operator and call base class' 
// assignment operator automatically

template <class IT, class NT>
FullyDistVec< IT,NT > &  FullyDistVec<IT,NT>::stealFrom(FullyDistVec<IT,NT> & victim)
{
	FullyDist<IT,NT,typename CombBLAS::disable_if< CombBLAS::is_boolean<NT>::value, NT >::type>::operator= (victim);	// to update glen and commGrid
	arr.swap(victim.arr);
	return *this;
}

template <class IT, class NT>
FullyDistVec< IT,NT > &  FullyDistVec<IT,NT>::operator+=(const FullyDistSpVec< IT,NT > & rhs)		
{
	IT spvecsize = rhs.getlocnnz();
	#ifdef _OPENMP
	#pragma omp parallel for
	#endif
	for(IT i=0; i< spvecsize; ++i)
	{
		if(arr[rhs.ind[i]] == NT()) // not set before
			arr[rhs.ind[i]] = rhs.num[i];
		else
			arr[rhs.ind[i]] += rhs.num[i];
	}
	return *this;
}

template <class IT, class NT>
void FullyDistVec<IT,NT>::Set(const FullyDistSpVec< IT,NT > & rhs)		
{
	IT spvecsize = rhs.getlocnnz();
	#ifdef _OPENMP
	#pragma omp parallel for
	#endif
	for(IT i=0; i< spvecsize; ++i)
	{
		arr[rhs.ind[i]] = rhs.num[i];
	}
}


template <class IT, class NT>
FullyDistVec< IT,NT > &  FullyDistVec<IT,NT>::operator-=(const FullyDistSpVec< IT,NT > & rhs)		
{
	IT spvecsize = rhs.getlocnnz();
	for(IT i=0; i< spvecsize; ++i)
	{
		arr[rhs.ind[i]] -= rhs.num[i];
	}
	return *this;
}


/**
  * Perform __binary_op(*this, v2) for every element in rhs, *this, 
  * which are of the same size. and write the result back to *this
  */ 
template <class IT, class NT>
template <typename _BinaryOperation>	
void FullyDistVec<IT,NT>::EWise(const FullyDistVec<IT,NT> & rhs,  _BinaryOperation __binary_op)
{
	transform ( arr.begin(), arr.end(), rhs.arr.begin(), arr.begin(), __binary_op );
};


template <class IT, class NT>
FullyDistVec<IT,NT> & FullyDistVec<IT, NT>::operator+=(const FullyDistVec<IT,NT> & rhs)
{
	if(this != &rhs)		
	{	
		if(!(*commGrid == *rhs.commGrid)) 		
		{
			cout << "Grids are not comparable elementwise addition" << endl; 
			MPI_Abort(MPI_COMM_WORLD, GRIDMISMATCH);
		}
		else 
		{
			EWise(rhs, std::plus<NT>());
		} 	
	}	
	return *this;
};

template <class IT, class NT>
FullyDistVec<IT,NT> & FullyDistVec<IT, NT>::operator-=(const FullyDistVec<IT,NT> & rhs)
{
	if(this != &rhs)		
	{	
		if(!(*commGrid == *rhs.commGrid)) 		
		{
			cout << "Grids are not comparable elementwise addition" << endl; 
			MPI_Abort(MPI_COMM_WORLD, GRIDMISMATCH);
		}
		else 
		{
			EWise(rhs, std::minus<NT>());
		} 	
	}	
	return *this;
};		

template <class IT, class NT>
bool FullyDistVec<IT,NT>::operator==(const FullyDistVec<IT,NT> & rhs) const
{
	ErrorTolerantEqual<NT> epsilonequal(EPSILON);
	int local = 1;
	local = (int) std::equal(arr.begin(), arr.end(), rhs.arr.begin(), epsilonequal );
	int whole = 1;
	MPI_Allreduce( &local, &whole, 1, MPI_INT, MPI_BAND, commGrid->GetWorld());
	return static_cast<bool>(whole);	
}

template <class IT, class NT>
template <typename _Predicate>
IT FullyDistVec<IT,NT>::Count(_Predicate pred) const
{
	IT local = count_if( arr.begin(), arr.end(), pred );
	IT whole = 0;
	MPI_Allreduce( &local, &whole, 1, MPIType<IT>(), MPI_SUM, commGrid->GetWorld());
	return whole;	
}

//! Returns a dense vector of global indices 
//! for which the predicate is satisfied
template <class IT, class NT>
template <typename _Predicate>
FullyDistVec<IT,IT> FullyDistVec<IT,NT>::FindInds(_Predicate pred) const
{
	FullyDistVec<IT,IT> found(commGrid);
	MPI_Comm World = commGrid->GetWorld();
	int nprocs = commGrid->GetSize();
	int rank = commGrid->GetRank();

	IT sizelocal = LocArrSize();
	IT sizesofar = LengthUntil();
	for(IT i=0; i<sizelocal; ++i)
	{
		if(pred(arr[i]))
		{
			found.arr.push_back(i+sizesofar);
		}
	}
	IT * dist = new IT[nprocs];
	IT nsize = found.arr.size(); 
	dist[rank] = nsize;
	MPI_Allgather(MPI_IN_PLACE, 1, MPIType<IT>(), dist, 1, MPIType<IT>(), World);
	IT lengthuntil = accumulate(dist, dist+rank, static_cast<IT>(0));
	found.glen = accumulate(dist, dist+nprocs, static_cast<IT>(0));

	// Although the found vector is not reshuffled yet, its glen and commGrid are set
	// We can call the Owner/MyLocLength/LengthUntil functions (to infer future distribution)

	// rebalance/redistribute
	int * sendcnt = new int[nprocs];
	fill(sendcnt, sendcnt+nprocs, 0);
	for(IT i=0; i<nsize; ++i)
	{
		IT locind;
		int owner = found.Owner(i+lengthuntil, locind);	
		++sendcnt[owner];
	}
	int * recvcnt = new int[nprocs];
	MPI_Alltoall(sendcnt, 1, MPI_INT, recvcnt, 1, MPI_INT, World); // share the counts

	int * sdispls = new int[nprocs];
	int * rdispls = new int[nprocs];
	sdispls[0] = 0;
	rdispls[0] = 0;
	for(int i=0; i<nprocs-1; ++i)
	{
		sdispls[i+1] = sdispls[i] + sendcnt[i];
		rdispls[i+1] = rdispls[i] + recvcnt[i];
	}
	IT totrecv = accumulate(recvcnt,recvcnt+nprocs, static_cast<IT>(0));
	vector<IT> recvbuf(totrecv);
			
	// data is already in the right order in found.arr
	MPI_Alltoallv(&(found.arr[0]), sendcnt, sdispls, MPIType<IT>(), &(recvbuf[0]), recvcnt, rdispls, MPIType<IT>(), World);
	found.arr.swap(recvbuf);
	delete [] dist;
	DeleteAll(sendcnt, recvcnt, sdispls, rdispls);

	return found;
}


//! Requires no communication because FullyDistSpVec (the return object)
//! is distributed based on length, not nonzero counts
template <class IT, class NT>
template <typename _Predicate>
FullyDistSpVec<IT,NT> FullyDistVec<IT,NT>::Find(_Predicate pred) const
{
	FullyDistSpVec<IT,NT> found(commGrid);
	size_t size = arr.size();
	for(size_t i=0; i<size; ++i)
	{
		if(pred(arr[i]))
		{
			found.ind.push_back( (IT) i);
			found.num.push_back(arr[i]);
		}
	}
	found.glen = glen;
	return found;	
}

template <class IT, class NT>
template <class HANDLER>
ifstream& FullyDistVec<IT,NT>::ReadDistribute (ifstream& infile, int master, HANDLER handler)
{
	FullyDistSpVec<IT,NT> tmpSpVec(commGrid);
	tmpSpVec.ReadDistribute(infile, master, handler);

	*this = tmpSpVec;
	return infile;
}

template <class IT, class NT>
template <class HANDLER>
void FullyDistVec<IT,NT>::SaveGathered(ofstream& outfile, int master, HANDLER handler, bool printProcSplits)
{
	FullyDistSpVec<IT,NT> tmpSpVec = *this;
	tmpSpVec.SaveGathered(outfile, master, handler, printProcSplits);
}

template <class IT, class NT>
void FullyDistVec<IT,NT>::SetElement (IT indx, NT numx)
{
	int rank = commGrid->GetRank();
	if (glen == 0) 
	{
		if(rank == 0)
			cout << "FullyDistVec::SetElement can't be called on an empty vector." << endl;
		return;
	}
	IT locind;
	int owner = Owner(indx, locind);
	if(commGrid->GetRank() == owner)
	{
		if (locind > (LocArrSize() -1))
		{
			cout << "FullyDistVec::SetElement cannot expand array" << endl;
		}
		else if (locind < 0)
		{
			cout << "FullyDistVec::SetElement local index < 0" << endl;
		}
		else
		{
			arr[locind] = numx;
		}
	}
}

template <class IT, class NT>
NT FullyDistVec<IT,NT>::GetElement (IT indx) const
{
	NT ret;
	MPI_Comm World = commGrid->GetWorld();
	int rank = commGrid->GetRank();
	if (glen == 0) 
	{
		if(rank == 0)
			cout << "FullyDistVec::GetElement can't be called on an empty vector." << endl;

		return NT();
	}
	IT locind;
	int owner = Owner(indx, locind);
	if(commGrid->GetRank() == owner)
	{
		if (locind > (LocArrSize() -1))
		{
			cout << "FullyDistVec::GetElement local index > size" << endl;
			ret = NT();

		}
		else if (locind < 0)
		{
			cout << "FullyDistVec::GetElement local index < 0" << endl;
			ret = NT();
		}
		else
		{
			ret = arr[locind];
		}
	}
	MPI_Bcast(&ret, 1, MPIType<NT>(), owner, World);
	return ret;
}

// Write to file using MPI-2
template <class IT, class NT>
void FullyDistVec<IT,NT>::DebugPrint()
{
	int nprocs, rank;
	MPI_Comm World = commGrid->GetWorld();
	MPI_Comm_rank(World, &rank);
	MPI_Comm_size(World, &nprocs);
	MPI_File thefile;
	MPI_File_open(World, "temp_fullydistvec", MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &thefile);    
	IT lengthuntil = LengthUntil();

	// The disp displacement argument specifies the position 
	// (absolute offset in bytes from the beginning of the file) 
	MPI_File_set_view(thefile, int64_t(lengthuntil * sizeof(NT)), MPIType<NT>(), MPIType<NT>(), "native", MPI_INFO_NULL);

	IT count = LocArrSize();	
	MPI_File_write(thefile, &(arr[0]), count, MPIType<NT>(), NULL);
	MPI_File_close(&thefile);
	
	// Now let processor-0 read the file and print
	IT * counts = new IT[nprocs];
	MPI_Gather(&count, 1, MPIType<IT>(), counts, 1, MPIType<IT>(), 0, World);	// gather at root=0
	if(rank == 0)
	{
		FILE * f = fopen("temp_fullydistvec", "r");
		if(!f)
		{
				cerr << "Problem reading binary input file\n";
				return;
		}
		IT maxd = *max_element(counts, counts+nprocs);
		NT * data = new NT[maxd];

		for(int i=0; i<nprocs; ++i)
		{
			// read counts[i] integers and print them
			size_t result = fread(data, sizeof(NT), counts[i],f);
			if (result != (unsigned)counts[i]) { cout << "Error in fread, only " << result << " entries read" << endl; }

			cout << "Elements stored on proc " << i << ": {" ;	
			for (int j = 0; j < counts[i]; j++)
			{
				cout << data[j] << ",";
			}
			cout << "}" << endl;
		}
		delete [] data;
		delete [] counts;
	}
}

template <class IT, class NT>
template <typename _UnaryOperation, typename IRRELEVANT_NT>
void FullyDistVec<IT,NT>::Apply(_UnaryOperation __unary_op, const FullyDistSpVec<IT,IRRELEVANT_NT> & mask)
{
	typename vector< IT >::const_iterator miter = mask.ind.begin();
	while (miter < mask.ind.end())
	{
		IT index = *miter++;
		arr[index] = __unary_op(arr[index]);
	}
}	

template <class IT, class NT>
template <typename _BinaryOperation, typename _BinaryPredicate, class NT2>
void FullyDistVec<IT,NT>::EWiseApply(const FullyDistVec<IT,NT2> & other, _BinaryOperation __binary_op, _BinaryPredicate _do_op, const bool useExtendedBinOp)
{
	if(*(commGrid) == *(other.commGrid))	
	{
		if(glen != other.glen)
		{
			ostringstream outs;
			outs << "Vector dimensions don't match (" << glen << " vs " << other.glen << ") for FullyDistVec::EWiseApply\n";
			SpParHelper::Print(outs.str());
			MPI_Abort(MPI_COMM_WORLD, GRIDMISMATCH);
		}
		else
		{
			typename vector< NT >::iterator thisIter = arr.begin();
			typename vector< NT2 >::const_iterator otherIter = other.arr.begin();
			while (thisIter < arr.end())
			{
				if (_do_op(*thisIter, *otherIter, false, false))
					*thisIter = __binary_op(*thisIter, *otherIter, false, false);
				thisIter++;
				otherIter++;
			}
		}
	}
	else
	{
		ostringstream outs;
		outs << "Grids are not comparable for FullyDistVec<IT,NT>::EWiseApply" << endl; 
		SpParHelper::Print(outs.str());
		MPI_Abort(MPI_COMM_WORLD, GRIDMISMATCH);
	}
}	

template <class IT, class NT>
template <typename _BinaryOperation, typename _BinaryPredicate, class NT2>
void FullyDistVec<IT,NT>::EWiseApply(const FullyDistSpVec<IT,NT2> & other, _BinaryOperation __binary_op, _BinaryPredicate _do_op, bool applyNulls, NT2 nullValue, const bool useExtendedBinOp)
{
	if(*(commGrid) == *(other.commGrid))	
	{
		if(glen != other.glen)
		{
			cerr << "Vector dimensions don't match (" << glen << " vs " << other.glen << ") for FullyDistVec::EWiseApply\n";
			MPI_Abort(MPI_COMM_WORLD, GRIDMISMATCH);
		}
		else
		{
			typename vector< IT >::const_iterator otherInd = other.ind.begin();
			typename vector< NT2 >::const_iterator otherNum = other.num.begin();
			
			if (applyNulls) // scan the entire dense vector and apply sparse elements as they appear
			{
				for(IT i=0; (unsigned)i < arr.size(); ++i)
				{
					if (otherInd == other.ind.end() || i < *otherInd)
					{
						if (_do_op(arr[i], nullValue, false, true))
							arr[i] = __binary_op(arr[i], nullValue, false, true);
					}
					else
					{
						if (_do_op(arr[i], *otherNum, false, false))
							arr[i] = __binary_op(arr[i], *otherNum, false, false);
						otherInd++;
						otherNum++;
					}
				}
			}
			else // scan the sparse vector only
			{
				while (otherInd < other.ind.end())
				{
					if (_do_op(arr[*otherInd], *otherNum, false, false))
						arr[*otherInd] = __binary_op(arr[*otherInd], *otherNum, false, false);
					otherInd++;
					otherNum++;
				}
			}
		}
	}
	else
	{
		cout << "Grids are not comparable elementwise apply" << endl; 
		MPI_Abort(MPI_COMM_WORLD, GRIDMISMATCH);
	}
}	


template <class IT, class NT>
FullyDistVec<IT, IT> FullyDistVec<IT, NT>::sort()
{
	MPI_Comm World = commGrid->GetWorld();
	FullyDistVec<IT,IT> temp(commGrid);
	IT nnz = LocArrSize(); 
	pair<NT,IT> * vecpair = new pair<NT,IT>[nnz];
	int nprocs = commGrid->GetSize();
	int rank = commGrid->GetRank();

	IT * dist = new IT[nprocs];
	dist[rank] = nnz;
	MPI_Allgather(MPI_IN_PLACE, 1, MPIType<IT>(), dist, 1, MPIType<IT>(), World);
	IT sizeuntil = LengthUntil();	// size = length, for dense vectors
	for(IT i=0; i< nnz; ++i)
	{
		vecpair[i].first = arr[i];	// we'll sort wrt numerical values
		vecpair[i].second = i + sizeuntil;	
	}
	SpParHelper::MemoryEfficientPSort(vecpair, nnz, dist, World);

	vector< IT > narr(nnz);
	for(IT i=0; i< nnz; ++i)
	{
		arr[i] = vecpair[i].first;	// sorted range (change the object itself)
		narr[i] = vecpair[i].second;	// inverse permutation stored as numerical values
	}
	delete [] vecpair;
	delete [] dist;

	temp.glen = glen;
	temp.arr = narr;
	return temp;
}
		

// Randomly permutes an already existing vector
template <class IT, class NT>
void FullyDistVec<IT,NT>::RandPerm()
{
	MPI_Comm World = commGrid->GetWorld();
	IT size = LocArrSize();
	pair<double,NT> * vecpair = new pair<double,NT>[size];
	int nprocs = commGrid->GetSize();
	int rank = commGrid->GetRank();
	IT * dist = new IT[nprocs];
	dist[rank] = size;
	MPI_Allgather(MPI_IN_PLACE, 1, MPIType<IT>(), dist, 1, MPIType<IT>(), World);
#ifdef DETERMINISTIC
	MTRand M(1);
#else
  	MTRand M;	// generate random numbers with Mersenne Twister
#endif
	for(int i=0; i<size; ++i)
	{
		vecpair[i].first = M.rand();
		vecpair[i].second = arr[i];	
	}
	// less< pair<T1,T2> > works correctly (sorts wrt first elements)	
	SpParHelper::MemoryEfficientPSort(vecpair, size, dist, World);
	vector< NT > nnum(size);
	for(int i=0; i<size; ++i)
		nnum[i] = vecpair[i].second;

	delete [] vecpair;
	delete [] dist;

	arr.swap(nnum);
}

// ABAB: In its current form, unless LengthUntil returns NT 
// this won't work reliably for anything other than integers
template <class IT, class NT>
void FullyDistVec<IT,NT>::iota(IT globalsize, NT first)
{
	glen = globalsize;
	IT length = MyLocLength();	// only needs glen to determine length
	
	arr.resize(length);
	SpHelper::iota(arr.begin(), arr.end(), LengthUntil() + first);	// global across processors
}

template <class IT, class NT>
FullyDistVec<IT,NT> FullyDistVec<IT,NT>::operator() (const FullyDistVec<IT,IT> & ri) const
{
	if(!(*commGrid == *ri.commGrid))
	{
		cout << "Grids are not comparable for dense vector subsref" << endl;
		return FullyDistVec<IT,NT>();
	}

	MPI_Comm World = commGrid->GetWorld();
	FullyDistVec<IT,NT> Indexed(commGrid, ri.glen, NT());	// length(Indexed) = length(ri)
	int nprocs = commGrid->GetSize();
	vector< vector< IT > > data_req(nprocs);	
	vector< vector< IT > > revr_map(nprocs);	// to put the incoming data to the correct location	

	IT riloclen = ri.LocArrSize();
	for(IT i=0; i < riloclen; ++i)
	{
		IT locind;
		int owner = Owner(ri.arr[i], locind);	// numerical values in ri are 0-based
		data_req[owner].push_back(locind);
		revr_map[owner].push_back(i);
	}
	IT * sendbuf = new IT[riloclen];
	int * sendcnt = new int[nprocs];
	int * sdispls = new int[nprocs];
	for(int i=0; i<nprocs; ++i)
		sendcnt[i] = (int) data_req[i].size();

	int * rdispls = new int[nprocs];
	int * recvcnt = new int[nprocs];
	MPI_Alltoall(sendcnt, 1, MPI_INT, recvcnt, 1, MPI_INT, World);  // share the request counts
	sdispls[0] = 0;
	rdispls[0] = 0;
	for(int i=0; i<nprocs-1; ++i)
	{
		sdispls[i+1] = sdispls[i] + sendcnt[i];
		rdispls[i+1] = rdispls[i] + recvcnt[i];
	}
	IT totrecv = accumulate(recvcnt,recvcnt+nprocs, static_cast<IT>(0));
	for(int i=0; i<nprocs; ++i)
	{
		copy(data_req[i].begin(), data_req[i].end(), sendbuf+sdispls[i]);
		vector<IT>().swap(data_req[i]);
	}

	IT * reversemap = new IT[riloclen];
	for(int i=0; i<nprocs; ++i)
	{
		copy(revr_map[i].begin(), revr_map[i].end(), reversemap+sdispls[i]);	// reversemap array is unique
		vector<IT>().swap(revr_map[i]);
	}

	IT * recvbuf = new IT[totrecv];
	MPI_Alltoallv(sendbuf, sendcnt, sdispls, MPIType<IT>(), recvbuf, recvcnt, rdispls, MPIType<IT>(), World);  // request data
	delete [] sendbuf;
		
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
	NT * databuf = new NT[riloclen];

	// the response counts are the same as the request counts 
	MPI_Alltoallv(databack, recvcnt, rdispls, MPIType<NT>(), databuf, sendcnt, sdispls, MPIType<NT>(), World);  // send data
	DeleteAll(rdispls, recvcnt, databack);

	// Now create the output from databuf
	// Indexed.arr is already allocated in contructor
	for(int i=0; i<nprocs; ++i)
	{
		for(int j=sdispls[i]; j< sdispls[i]+sendcnt[i]; ++j)
		{
			Indexed.arr[reversemap[j]] = databuf[j];
		}
	}
	DeleteAll(sdispls, sendcnt, databuf,reversemap);
	return Indexed;
}

template <class IT, class NT>
void FullyDistVec<IT,NT>::PrintInfo(string vectorname) const
{
	IT totl = TotalLength();
	if (commGrid->GetRank() == 0)		
		cout << "As a whole, " << vectorname << " has length " << totl << endl; 
}
