#include <limits>
#include "SpParVec.h"
#include "SpDefs.h"
#include "SpHelper.h"
using namespace std;

template <class IT, class NT>
SpParVec<IT, NT>::SpParVec ( shared_ptr<CommGrid> grid): commGrid(grid), length(0), NOT_FOUND(numeric_limits<NT>::min())
{
	if(commGrid->GetRankInProcRow() == commGrid->GetRankInProcCol())
		diagonal = true;
	else
		diagonal = false;	
};

template <class IT, class NT>
SpParVec<IT, NT>::SpParVec ( shared_ptr<CommGrid> grid, IT loclen): commGrid(grid), NOT_FOUND(numeric_limits<NT>::min())
{
	if(commGrid->GetRankInProcRow() == commGrid->GetRankInProcCol())
	{
		diagonal = true;
		length = loclen;
	}
	else
	{
		diagonal = false;	
		length = 0;
	}
};

template <class IT, class NT>
SpParVec<IT, NT>::SpParVec (): length(0), NOT_FOUND(numeric_limits<NT>::min())
{
	commGrid.reset(new CommGrid(MPI_COMM_WORLD, 0, 0));
	
	if(commGrid->GetRankInProcRow() == commGrid->GetRankInProcCol())
		diagonal = true;
	else
		diagonal = false;	
};

template <class IT, class NT>
SpParVec<IT, NT>::SpParVec (IT loclen): NOT_FOUND(numeric_limits<NT>::min())
{
	commGrid.reset(new CommGrid(MPI_COMM_WORLD, 0, 0));
	if(commGrid->GetRankInProcRow() == commGrid->GetRankInProcCol())
	{
		diagonal = true;
		length = loclen;
	}
	else
	{
		diagonal = false;	
		length = 0;
	}
}

template <class IT, class NT>
void SpParVec<IT,NT>::stealFrom(SpParVec<IT,NT> & victim)
{
	commGrid.reset(new CommGrid(*(victim.commGrid)));		
	ind.swap(victim.ind);
	num.swap(victim.num);
	diagonal = victim.diagonal;
	length = victim.length;
}

template <class IT, class NT>
NT SpParVec<IT,NT>::operator[](IT indx) const
{
	MPI_Comm DiagWorld = commGrid->GetDiagWorld();
	NT val;
	if(DiagWorld != MPI_COMM_NULL) // Diagonal processors only
	{
		int rank, size;
		MPI_Comm_rank(DiagWorld, &rank); 
		MPI_Comm_size(DiagWorld, &size);
		IT dgrank = (IT) rank;
		IT nprocs = (IT) size;	

		IT n_perproc = getTypicalLocLength();
		IT offset = dgrank * n_perproc;

		IT owner = (indx) / n_perproc;	
		owner = std::min(owner, nprocs-1);	// find its owner 
		NT diagval;
		if(owner == dgrank)
		{
			IT locindx = indx-offset; 
			typename vector<IT>::const_iterator it = lower_bound(ind.begin(), ind.end(), locindx);	// ind is a sorted vector
			if(it != ind.end() && locindx == (*it))	// found
			{
				diagval = num[it-ind.begin()];
			}
			else
			{
				diagval = NOT_FOUND;	// return NULL
			}
		}
		MPI_Bcast(&diagval, 1, MPIType<NT>(), owner, DiagWorld);
		val = diagval;
	}

	IT diaginrow = commGrid->GetDiagOfProcRow();
	MPI_Bcast(&val, 1, MPIType<NT>(), diaginrow, commGrid->GetRowWorld());
	return val;
}

//! Performs almost no communication other than getnnz()
//! Indexing is performed 0-based 
template <class IT, class NT>
void SpParVec<IT,NT>::SetElement (IT indx, NT numx)
{
	MPI_Comm DiagWorld = commGrid->GetDiagWorld();
	if(DiagWorld != MPI_COMM_NULL) // Diagonal processors only
	{
		int dgrank, nprocs;
		MPI_Comm_rank(DiagWorld, &dgrank); 
		MPI_Comm_size(DiagWorld, &nprocs);

		IT n_perproc = getTypicalLocLength();
		int owner = std::min(static_cast<int>(indx/n_perproc), nprocs-1);
		
		if(owner == dgrank) // insert if this process is the owner
		{
			IT locindx = indx-(dgrank*n_perproc); 
			typename vector<IT>::iterator iter = lower_bound(ind.begin(), ind.end(), locindx);	
			if(iter == ind.end())	// beyond limits, insert from back
			{
				ind.push_back(locindx);
				num.push_back(numx);
			}
			else if (locindx < *iter)	// not found, insert in the middle
			{
				// the order of insertions is crucial
				// if we first insert to ind, then ind.begin() is invalidated !
				num.insert(num.begin() + (iter-ind.begin()), numx);
				ind.insert(iter, locindx);
			}
			else // found
			{
				*(num.begin() + (iter-ind.begin())) = numx;
			}
		}
	}
}

/**
 * The distribution and length are inherited from ri
 * Example: This is [{1,n1},{4,n4},{7,n7},{8,n8},{9,n9}] with P_00 owning {1,4} and P_11 rest
 * Assume ri = [{1,4},{2,1}] is distributed as one element per processor
 * Then result has length 2, distrubuted one element per processor
 * TODO: Indexing (with this given semantics) would just work with
 * the prototype operator() (const DenseParVec<IT,IT> & ri) 
 * because this code makes no reference to ri.ind() 
**/
template <class IT, class NT>
SpParVec<IT,NT> SpParVec<IT,NT>::operator() (const SpParVec<IT,IT> & ri) const
{
	MPI_Comm DiagWorld = commGrid->GetDiagWorld();
	SpParVec<IT,NT> Indexed(commGrid);
	if(DiagWorld != MPI_COMM_NULL) // Diagonal processors only
	{
		int dgrank, nprocs;
		MPI_Comm_rank(DiagWorld, &dgrank); 
		MPI_Comm_size(DiagWorld, &nprocs);
		IT n_perproc = getTypicalLocLength();
		vector< vector<IT> > data_req(nprocs);
		for(IT i=0; i < ri.num.size(); ++i)
		{
			int owner = (ri.num[i]) / n_perproc;	// numerical values in ri are 0-based
			int rec = std::min(owner, nprocs-1);	// find its owner 
			data_req[rec].push_back(ri.num[i] - (rec * n_perproc));
		}
		IT * sendbuf = new IT[ri.num.size()];
		int * sendcnt = new int[nprocs];
		int * sdispls = new int[nprocs];
		for(int i=0; i<nprocs; ++i)
			sendcnt[i] = data_req[i].size();

		int * rdispls = new int[nprocs];
		int * recvcnt = new int[nprocs];
		MPI_Alltoall(sendcnt, 1, MPI_INT, recvcnt, 1, MPI_INT, DiagWorld);      // share the request counts

		sdispls[0] = 0;
		rdispls[0] = 0;
		for(int i=0; i<nprocs-1; ++i)
		{
			sdispls[i+1] = sdispls[i] + sendcnt[i];
			rdispls[i+1] = rdispls[i] + recvcnt[i];
		}
		IT totrecv = accumulate(recvcnt,recvcnt+nprocs,0);
		IT * recvbuf = new IT[totrecv];

		for(int i=0; i<nprocs; ++i)
			copy(data_req[i].begin(), data_req[i].end(), sendbuf+sdispls[i]);
	
		MPI_Alltoallv(sendbuf, sendcnt, sdispls, MPIType<IT>(), recvbuf, recvcnt, rdispls, MPIType<IT>(), DiagWorld);  // request data
		
		// We will return the requested data, 
		// our return can be at most as big as the request
		// and smaller if we are missing some elements 
		IT * indsback = new IT[totrecv];
		NT * databack = new NT[totrecv];		

		int * ddispls = new int[nprocs];
		copy(rdispls, rdispls+nprocs, ddispls);
		for(int i=0; i<nprocs; ++i)
		{
			// this is not the most efficient method because it scans ind vector nprocs = sqrt(p) times
			IT * it = set_intersection(recvbuf+rdispls[i], recvbuf+rdispls[i]+recvcnt[i], ind.begin(), ind.end(), indsback+rdispls[i]);
			recvcnt[i] = (it - (indsback+rdispls[i]));	// update with size of the intersection
	
			IT vi = 0;
			for(int j = rdispls[i]; j < rdispls[i] + recvcnt[i]; ++j)	// fetch the numerical values
			{
				// indsback is a subset of ind
				while(indsback[j] > ind[vi]) 
					++vi;
				databack[j] = num[vi++];
			}
		}
		
		DeleteAll(recvbuf, ddispls);
		NT * databuf = new NT[ri.num.size()];

		MPI_Alltoall(recvcnt, 1, MPI_INT, sendcnt, 1, MPI_INT, DiagWorld);      // share the response counts, overriding request counts
		MPI_Alltoallv(indsback, recvcnt, rdispls, MPIType<IT>(), sendbuf, sendcnt, sdispls, MPIType<IT>(), DiagWorld);  // send data
		MPI_Alltoallv(databack, recvcnt, rdispls, MPIType<NT>(), databuf, sendcnt, sdispls, MPIType<NT>(), DiagWorld);  // send data

		DeleteAll(rdispls, recvcnt, indsback, databack);

		// Now create the output from databuf 
		for(int i=0; i<nprocs; ++i)
		{
			// data will come globally sorted from processors 
			// i.e. ind owned by proc_i is always smaller than 
			// ind owned by proc_j for j < i
			for(int j=sdispls[i]; j< sdispls[i]+sendcnt[i]; ++j)
			{
				Indexed.ind.push_back(sendbuf[j]);
				Indexed.num.push_back(databuf[j]);
			}
		}
		Indexed.length = ri.length;
		DeleteAll(sdispls, sendcnt, sendbuf, databuf);
	}
	return Indexed;
}

template <class IT, class NT>
void SpParVec<IT,NT>::iota(IT size, NT first)
{
	MPI_Comm DiagWorld = commGrid->GetDiagWorld();
	if(DiagWorld != MPI_COMM_NULL) // Diagonal processors only
	{
		int dgrank, nprocs;
		MPI_Comm_rank(DiagWorld, &dgrank); 
		MPI_Comm_size(DiagWorld, &nprocs);
		IT n_perproc = size / nprocs;

		length = (dgrank != nprocs-1) ? n_perproc: (size - (n_perproc * (nprocs-1)));
		ind.resize(length);
		num.resize(length);
		SpHelper::iota(ind.begin(), ind.end(), 0);	// offset'd within processors
		SpHelper::iota(num.begin(), num.end(), (dgrank * n_perproc) + first);	// global across processors
	}
}

template <class IT, class NT>
SpParVec<IT, IT> SpParVec<IT, NT>::sort()
{
	MPI_Comm DiagWorld = commGrid->GetDiagWorld();
	SpParVec<IT,IT> temp(commGrid);
	if(DiagWorld != MPI_COMM_NULL) // Diagonal processors only
	{
		IT nnz = ind.size(); 
		pair<IT,IT> * vecpair = new pair<IT,IT>[nnz];

		int dgrank, nprocs;
		MPI_Comm_rank(DiagWorld, &dgrank); 
		MPI_Comm_size(DiagWorld, &nprocs);
		
		IT * dist = new IT[nprocs];
		dist[dgrank] = nnz;
		MPI_Allgather(MPI_IN_PLACE, 1, MPIType<IT>(), dist, 1, MPIType<IT>(), DiagWorld);
		IT lengthuntil = accumulate(dist, dist+dgrank, static_cast<IT>(0));
		for(size_t i=0; i<nnz; ++i)
		{
			vecpair[i].first = num[i];	// we'll sort wrt numerical values
			vecpair[i].second = ind[i] + lengthuntil;	// return 0-based indices	
		}
		long * dist_in = new long[nprocs];
		for(int i=0; i< nprocs; ++i)	dist_in[i] = (long) dist[i];	
    		vpsort::parallel_sort (vecpair, vecpair + nnz,  dist_in, DiagWorld);
		DeleteAll(dist_in,dist);

		vector< IT > nind(nnz);
		vector< IT > nnum(nnz);
		for(size_t i=0; i<nnz; ++i)
		{
			num[i] = vecpair[i].first;	// sorted range
			nind[i] = ind[i];		// make sure the sparsity distribution is the same
			nnum[i] = vecpair[i].second;	// inverse permutation stored as numerical values
		}
		delete [] vecpair;

		temp.length = length;
		temp.ind = nind;
		temp.num = nnum;
	}
	return temp;
}
		
template <class IT, class NT>
SpParVec<IT,NT> & SpParVec<IT, NT>::operator+=(const SpParVec<IT,NT> & rhs)
{
	if(this != &rhs)		
	{	
		if(diagonal)	// Only the diagonal processors hold values
		{	
			if(length != rhs.length)
			{
				cerr << "Vector dimensions don't match for addition\n";
				return *this; 	
			}
			IT lsize = ind.size();
			IT rsize = rhs.ind.size();

			vector< IT > nind;
			vector< NT > nnum;
			nind.reserve(lsize+rsize);
			nnum.reserve(lsize+rsize);

			IT i=0, j=0;
			while(i < lsize && j < rsize)
			{
				// assignment won't change the size of vector, push_back is necessary
				if(ind[i] > rhs.ind[j])
				{	
					nind.push_back( rhs.ind[j] );
					nnum.push_back( rhs.num[j++] );
				}
				else if(ind[i] < rhs.ind[j])
				{
					nind.push_back( ind[i] );
					nnum.push_back( num[i++] );
				}
				else
				{
					nind.push_back( ind[i] );
					nnum.push_back( num[i++] + rhs.num[j++] );
				}
			}
			while( i < lsize)	// rhs was depleted first
			{
				nind.push_back( ind[i] );
				nnum.push_back( num[i++] );
			}
			while( j < rsize) 	// *this was depleted first
			{
				nind.push_back( rhs.ind[j] );
				nnum.push_back( rhs.num[j++] );
			}
			ind.swap(nind);		// ind will contain the elements of nind with capacity shrunk-to-fit size
			num.swap(nnum); 	
		}
	}	
	else
	{		
		if(diagonal)
		{
			typename vector<NT>::iterator it;
			for(it = num.begin(); it != num.end(); ++it)
				(*it) *= 2;
		}
	}
	return *this;
};	
template <class IT, class NT>
SpParVec<IT,NT> & SpParVec<IT, NT>::operator-=(const SpParVec<IT,NT> & rhs)
{
	if(this != &rhs)		
	{	
		if(diagonal)	// Only the diagonal processors hold values
		{
			if(length != rhs.length)
			{
				cerr << "Vector dimensions don't match for addition\n";
				return *this; 	
			}
			IT lsize = ind.size();
			IT rsize = rhs.ind.size();

			vector< IT > nind;
			vector< NT > nnum;
			nind.reserve(lsize+rsize);
			nnum.reserve(lsize+rsize);

			IT i=0, j=0;
			while(i < lsize && j < rsize)
			{
				// assignment won't change the size of vector, push_back is necessary
				if(ind[i] > rhs.ind[j])
				{	
					nind.push_back( rhs.ind[j] );
					nnum.push_back( -rhs.num[j++] );
				}
				else if(ind[i] < rhs.ind[j])
				{
					nind.push_back( ind[i] );
					nnum.push_back( num[i++] );
				}
				else
				{
					nind.push_back( ind[i] );
					nnum.push_back( num[i++] - rhs.num[j++] );
				}
			}
			while( i < lsize)	// rhs was depleted first
			{
				nind.push_back( ind[i] );
				nnum.push_back( num[i++] );
			}
			while( j < rsize) 	// *this was depleted first
			{
				nind.push_back( rhs.ind[j] );
				nnum.push_back( -rhs.num[j++] );
			}
			ind.swap(nind);		// ind will contain the elements of nind with capacity shrunk-to-fit size
			num.swap(nnum); 	
		}
	}	
	else
	{
		if(diagonal)
		{
			ind.clear();
			num.clear();
		}
	}
	return *this;
};	

//! Called on an existing object
template <class IT, class NT>
ifstream& SpParVec<IT,NT>::ReadDistribute (ifstream& infile, int master)
{
	length = 0;	// will be updated for diagonal processors
	IT total_n, total_nnz, n_perproc;
	MPI_Comm DiagWorld = commGrid->GetDiagWorld();
	if(DiagWorld != MPI_COMM_NULL)	// Diagonal processors only
	{
		int neighs, diagrank;
		MPI_Comm_size(DiagWorld, &neighs);      // number of neighbors along diagonal (including oneself)
		MPI_Comm_rank(DiagWorld, &diagrank);

		int buffperneigh = MEMORYINBYTES / (neighs * (sizeof(IT) + sizeof(NT)));
		int * displs = new int[neighs];
		for (int i=0; i<neighs; ++i)
			displs[i] = i*buffperneigh;

		int * curptrs; 
		int recvcount;
		IT * inds; 
		NT * vals;

		if(diagrank == master)	// 1 processor only
		{		
			inds = new IT [ buffperneigh * neighs ];
			vals = new NT [ buffperneigh * neighs ];

			curptrs = new int[neighs]; 
			fill_n(curptrs, neighs, 0);	// fill with zero
		
			if (infile.is_open())
			{
				infile.clear();
				infile.seekg(0);
				infile >> total_n >> total_nnz;
				n_perproc = total_n / neighs;	// the last proc gets the extras
				MPI_Bcast(&total_n, 1, MPIType<IT>(), master, DiagWorld);

				IT tempind;
				NT tempval;
				IT cnz = 0;
				while ( (!infile.eof()) && cnz < total_nnz)
				{
					infile >> tempind;
					infile >> tempval;
					tempind--;

					int rec = std::min((int)(tempind / n_perproc), neighs-1);	// recipient processor along the diagonal
					inds[ rec * buffperneigh + curptrs[rec] ] = tempind;
					vals[ rec * buffperneigh + curptrs[rec] ] = tempval;
					++ (curptrs[rec]);				

					if(curptrs[rec] == buffperneigh || (cnz == (total_nnz-1)) )		// one buffer is full, or file is done !
					{
						// first, send the receive counts ...
						MPI_Scatter(curptrs, 1, MPI_INT, &recvcount, 1, MPI_INT, master, DiagWorld);

						// generate space for own recv data ... (use arrays because vector<bool> is cripled, if NT=bool)
						IT * tempinds = new IT[recvcount];
						NT * tempvals = new NT[recvcount];
					
						// then, send all buffers that to their recipients ...
						MPI_Scatterv(inds, curptrs, displs, MPIType<IT>(), tempinds, recvcount,  MPIType<IT>(), master, DiagWorld);
						MPI_Scatterv(vals, curptrs, displs, MPIType<NT>(), tempvals, recvcount,  MPIType<NT>(), master, DiagWorld);

						// now push what is ours to tuples
						IT offset = master * n_perproc;
						for(IT i=0; i< recvcount; ++i)
						{					
							ind.push_back( tempinds[i]-offset );
							num.push_back( tempvals[i] );
						}

						// reset current pointers so that we can reuse {inds,vals} buffers
						fill_n(curptrs, neighs, 0);
						DeleteAll(tempinds, tempvals);
					}
					++ cnz;
				}
				assert (cnz == total_nnz);
			
				// Signal the end of file to other processors along the diagonal
				fill_n(curptrs, neighs, numeric_limits<int>::max());	
				MPI_Scatter(curptrs, 1, MPI_INT, &recvcount, 1, MPI_INT, master, DiagWorld);
			}
			else	// input file does not exist !
			{
				total_n = 0;	
				MPI_Bcast(&total_n, 1, MPIType<IT>(), master, DiagWorld);
			}
			DeleteAll(inds,vals, curptrs);
		}
		else 	 	// (r-1) processors on the diagonal
		{
			MPI_Bcast(&total_n, 1, MPIType<IT>(), master, DiagWorld);
			n_perproc = total_n / neighs;

			while(total_n > 0)	// otherwise, input file do not exist
			{
				// first receive the receive counts ...
				MPI_Scatter(curptrs, 1, MPI_INT, &recvcount, 1, MPI_INT, master, DiagWorld);

				if( recvcount == numeric_limits<int>::max())
					break;
	
				// create space for incoming data ... 
				IT * tempinds = new IT[recvcount];
				NT * tempvals = new NT[recvcount];

				// receive actual data ... (first 4 arguments are ignored in the receiver side)
				MPI_Scatterv(inds, curptrs, displs, MPIType<IT>(), tempinds, recvcount,  MPIType<IT>(), master, DiagWorld);
				MPI_Scatterv(vals, curptrs, displs, MPIType<NT>(), tempvals, recvcount,  MPIType<NT>(), master, DiagWorld);

				// now push what is ours to tuples
				IT offset = diagrank * n_perproc;
				for(IT i=0; i< recvcount; ++i)
				{					
					ind.push_back( tempinds[i]-offset );
					num.push_back( tempvals[i] );
				}

				DeleteAll(tempinds, tempvals);
			}
		}
		delete [] displs;
 		length = (diagrank != neighs-1) ? n_perproc: (total_n - (n_perproc * (neighs-1)));
	}	
	MPI_Barrier(commGrid->GetWorld());
	return infile;
}

template <class IT, class NT>
IT SpParVec<IT,NT>::getTotalLength(MPI_Comm & comm) const
{
	IT totlen = 0;
	if(comm != MPI_COMM_NULL)
	{
		MPI_Allreduce( &length, & totlen, 1, MPIType<IT>(), MPI_SUM, comm);
	}
	return totlen;
}

template <class IT, class NT>
IT SpParVec<IT,NT>::getTypicalLocLength() const
{
	IT n_perproc = 0 ;
        MPI_Comm DiagWorld = commGrid->GetDiagWorld();
        if(DiagWorld != MPI_COMM_NULL) // Diagonal processors only
        {
		int dgrank, nprocs;
		MPI_Comm_rank(DiagWorld, &dgrank); 
		MPI_Comm_size(DiagWorld, &nprocs);
                n_perproc = length;
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
template <typename _BinaryOperation>
NT SpParVec<IT,NT>::Reduce(_BinaryOperation __binary_op, NT init)
{
	// std::accumulate returns init for empty sequences
	// the semantics are init + num[0] + ... + num[n]
	NT localsum = std::accumulate( num.begin(), num.end(), init, __binary_op);

	NT totalsum = init;
	MPI_Allreduce( &localsum, &totalsum, 1, MPIType<NT>(), MPIOp<_BinaryOperation, NT>::op(), commGrid->GetWorld());
	return totalsum;
}

template <class IT, class NT>
void SpParVec<IT,NT>::PrintInfo(string vectorname) const
{
	IT nznz = getnnz();
	IT totl = getTotalLength();

	if(diagonal)
	{
		if (commGrid->GetRank() == 0)	
			cout << "As a whole, " << vectorname << " has: " << nznz << " nonzeros and length " << totl << endl; 
	}
}

template <class IT, class NT>
void SpParVec<IT,NT>::DebugPrint()
{
	MPI_Comm DiagWorld = commGrid->GetDiagWorld();
	if(DiagWorld != MPI_COMM_NULL) // Diagonal processors only
	{
		int dgrank, nprocs;
		MPI_Comm_rank(DiagWorld, &dgrank); 
		MPI_Comm_size(DiagWorld, &nprocs);

		IT* all_nnzs = new IT[nprocs];
		
		all_nnzs[dgrank] = ind.size();
		MPI_Allgather(MPI_IN_PLACE, 1, MPIType<IT>(), all_nnzs, 1, MPIType<IT>(), DiagWorld);
		IT offset = 0;
		
		for (int i = 0; i < nprocs; i++)
		{
			if (i == dgrank)
			{
				cout << "stored on proc " << dgrank << "," << dgrank << ":" << endl;
				
				for (int j = 0; j < ind.size(); j++)
				{
					cout << "Element #" << (j+offset) << ": [" << (ind[j]) << "] = " << num[j] << endl;
				}
			}
			offset += all_nnzs[i];
			MPI_Barrier(DiagWorld);
		}
		MPI_Barrier(DiagWorld);
		if (dgrank == 0)
			cout << "total size: " << offset << endl;
	}
}
