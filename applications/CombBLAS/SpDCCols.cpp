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

#include "SpDCCols.h"
#include "Deleter.h"
#include <algorithm>
#include <functional>
#include <vector>
#include <climits>
#include <iomanip>
#include <cassert>

using namespace std;

/****************************************************************************/
/********************* PUBLIC CONSTRUCTORS/DESTRUCTORS **********************/
/****************************************************************************/

template <class IT, class NT>
const IT SpDCCols<IT,NT>::esscount = static_cast<IT>(4);


template <class IT, class NT>
SpDCCols<IT,NT>::SpDCCols():dcsc(NULL), m(0), n(0), nnz(0), splits(0){
}

// Allocate all the space necessary
template <class IT, class NT>
SpDCCols<IT,NT>::SpDCCols(IT size, IT nRow, IT nCol, IT nzc)
:m(nRow), n(nCol), nnz(size), splits(0)
{
	if(nnz > 0)
		dcsc = new Dcsc<IT,NT>(nnz, nzc);
	else
		dcsc = NULL; 
}

template <class IT, class NT>
SpDCCols<IT,NT>::~SpDCCols()
{
	if(nnz > 0)
	{
		if(dcsc != NULL) 
		{	
			if(splits > 0)
			{
				for(int i=0; i<splits; ++i)
					delete dcscarr[i];
				delete [] dcscarr;
			}
			else
			{
				delete dcsc;	
			}
		}
	}
}

// Copy constructor (constructs a new object. i.e. this is NEVER called on an existing object)
// Derived's copy constructor can safely call Base's default constructor as base has no data members 
template <class IT, class NT>
SpDCCols<IT,NT>::SpDCCols(const SpDCCols<IT,NT> & rhs)
: m(rhs.m), n(rhs.n), nnz(rhs.nnz), splits(rhs.splits)
{
	if(splits > 0)
	{
		for(int i=0; i<splits; ++i)
			CopyDcsc(rhs.dcscarr[i]);
	}
	else
	{
		CopyDcsc(rhs.dcsc);
	}
}

/** 
 * Constructor for converting SpTuples matrix -> SpDCCols (may use a private memory heap)
 * @param[in] 	rhs if transpose=true, 
 *	\n		then rhs is assumed to be a row sorted SpTuples object 
 *	\n		else rhs is assumed to be a column sorted SpTuples object
 **/
template <class IT, class NT>
SpDCCols<IT,NT>::SpDCCols(const SpTuples<IT,NT> & rhs, bool transpose)
: m(rhs.m), n(rhs.n), nnz(rhs.nnz), splits(0)
{	 
	
	if(nnz == 0)	// m by n matrix of complete zeros
	{
		if(transpose) swap(m,n);
		dcsc = NULL;	
	} 
	else
	{
		if(transpose)
		{
			swap(m,n);
			IT localnzc = 1;
			for(IT i=1; i< rhs.nnz; ++i)
			{
				if(rhs.rowindex(i) != rhs.rowindex(i-1))
				{
					++localnzc;
	 			}
	 		}
			dcsc = new Dcsc<IT,NT>(rhs.nnz,localnzc);	
			dcsc->jc[0]  = rhs.rowindex(0); 
			dcsc->cp[0] = 0;

			for(IT i=0; i<rhs.nnz; ++i)
	 		{
				dcsc->ir[i]  = rhs.colindex(i);		// copy rhs.jc to ir since this transpose=true
				dcsc->numx[i] = rhs.numvalue(i);
			}

			IT jspos  = 1;		
			for(IT i=1; i<rhs.nnz; ++i)
			{
				if(rhs.rowindex(i) != dcsc->jc[jspos-1])
				{
					dcsc->jc[jspos] = rhs.rowindex(i);	// copy rhs.ir to jc since this transpose=true
					dcsc->cp[jspos++] = i;
				}
			}		
			dcsc->cp[jspos] = rhs.nnz;
	 	}
		else
		{
			IT localnzc = 1;
			for(IT i=1; i<rhs.nnz; ++i)
			{
				if(rhs.colindex(i) != rhs.colindex(i-1))
				{
					++localnzc;
				}
			}
			dcsc = new Dcsc<IT,NT>(rhs.nnz,localnzc);	
			dcsc->jc[0]  = rhs.colindex(0); 
			dcsc->cp[0] = 0;

			for(IT i=0; i<rhs.nnz; ++i)
			{
				dcsc->ir[i]  = rhs.rowindex(i);		// copy rhs.ir to ir since this transpose=false
				dcsc->numx[i] = rhs.numvalue(i);
			}

			IT jspos = 1;		
			for(IT i=1; i<rhs.nnz; ++i)
			{
				if(rhs.colindex(i) != dcsc->jc[jspos-1])
				{
					dcsc->jc[jspos] = rhs.colindex(i);	// copy rhs.jc to jc since this transpose=true
					dcsc->cp[jspos++] = i;
				}
			}		
			dcsc->cp[jspos] = rhs.nnz;
		}
	} 
}


/****************************************************************************/
/************************** PUBLIC OPERATORS ********************************/
/****************************************************************************/

/**
 * The assignment operator operates on an existing object
 * The assignment operator is the only operator that is not inherited.
 * But there is no need to call base's assigment operator as it has no data members
 */
template <class IT, class NT>
SpDCCols<IT,NT> & SpDCCols<IT,NT>::operator=(const SpDCCols<IT,NT> & rhs)
{
	// this pointer stores the address of the class instance
	// check for self assignment using address comparison
	if(this != &rhs)		
	{
		if(dcsc != NULL && nnz > 0)
		{
			delete dcsc;
		}
		if(rhs.dcsc != NULL)	
		{
			dcsc = new Dcsc<IT,NT>(*(rhs.dcsc));
			nnz = rhs.nnz;
		}
		else
		{
			dcsc = NULL;
			nnz = 0;
		}
		
		m = rhs.m; 
		n = rhs.n;
		splits = rhs.splits;
	}
	return *this;
}

template <class IT, class NT>
SpDCCols<IT,NT> & SpDCCols<IT,NT>::operator+= (const SpDCCols<IT,NT> & rhs)
{
	// this pointer stores the address of the class instance
	// check for self assignment using address comparison
	if(this != &rhs)		
	{
		if(m == rhs.m && n == rhs.n)
		{
			if(rhs.nnz == 0)
			{
				return *this;
			}
			else if(nnz == 0)
			{
				dcsc = new Dcsc<IT,NT>(*(rhs.dcsc));
				nnz = dcsc->nz;
			}
			else
			{
				(*dcsc) += (*(rhs.dcsc));
				nnz = dcsc->nz;
			}		
		}
		else
		{
			cout<< "Not addable: " << m  << "!=" << rhs.m << " or " << n << "!=" << rhs.n <<endl;		
		}
	}
	else
	{
		cout<< "Missing feature (A+A): Use multiply with 2 instead !"<<endl;	
	}
	return *this;
}

template <class IT, class NT>
template <typename _UnaryOperation>
SpDCCols<IT,NT>* SpDCCols<IT,NT>::Prune(_UnaryOperation __unary_op, bool inPlace)
{
	if(nnz > 0)
	{
		Dcsc<IT,NT>* ret = dcsc->Prune (__unary_op, inPlace);
		if (inPlace)
		{
			nnz = dcsc->nz;
	
			if(nnz == 0) 
			{	
				delete dcsc;
				dcsc = NULL;
			}
			return NULL;
		}
		else
		{
			// wrap the new pruned Dcsc into a new SpDCCols
			SpDCCols<IT,NT>* retcols = new SpDCCols<IT, NT>();
			retcols->dcsc = ret;
			retcols->nnz = retcols->dcsc->nz;
			retcols->n = n;
			retcols->m = m;
			return retcols;
		}
	}
	else
	{
		if (inPlace)
		{
			return NULL;
		}
		else
		{
			SpDCCols<IT,NT>* retcols = new SpDCCols<IT, NT>();
			retcols->dcsc = NULL;
			retcols->nnz = 0;
			retcols->n = n;
			retcols->m = m;
			return retcols;
		}
	}
}

template <class IT, class NT>
void SpDCCols<IT,NT>::EWiseMult (const SpDCCols<IT,NT> & rhs, bool exclude)
{
	if(this != &rhs)		
	{
		if(m == rhs.m && n == rhs.n)
		{
			if(rhs.nnz == 0)
			{
				if(exclude)	// then we don't exclude anything
				{
					return;
				}
				else	// A .* zeros() is zeros()
				{
					*this = SpDCCols<IT,NT>(0,m,n,0);	// completely reset the matrix
				}
			}
			else if (rhs.nnz != 0 && nnz != 0)
			{
				dcsc->EWiseMult (*(rhs.dcsc), exclude);
				nnz = dcsc->nz;
				if(nnz == 0 )
					dcsc = NULL;
			}		
		}
		else
		{
			cout<< "Matrices do not conform for A .* op(B) !"<<endl;		
		}
	}
	else
	{
		cout<< "Missing feature (A .* A): Use Square_EWise() instead !"<<endl;	
	}
}

/**
 * @Pre {scaler should NOT contain any zero entries}
 */
template <class IT, class NT>
void SpDCCols<IT,NT>::EWiseScale(NT ** scaler, IT m_scaler, IT n_scaler)
{
	if(m == m_scaler && n == n_scaler)
	{
		if(nnz > 0)
			dcsc->EWiseScale (scaler);
	}
	else
	{
		cout<< "Matrices do not conform for EWiseScale !"<<endl;		
	}
}	


/****************************************************************************/
/********************* PUBLIC MEMBER FUNCTIONS ******************************/
/****************************************************************************/

template <class IT, class NT>
void SpDCCols<IT,NT>::CreateImpl(const vector<IT> & essentials)
{
	assert(essentials.size() == esscount);
	nnz = essentials[0];
	m = essentials[1];
	n = essentials[2];

	if(nnz > 0)
		dcsc = new Dcsc<IT,NT>(nnz,essentials[3]);
	else
		dcsc = NULL; 
}

template <class IT, class NT>
void SpDCCols<IT,NT>::CreateImpl(IT size, IT nRow, IT nCol, tuple<IT, IT, NT> * mytuples)
{
	SpTuples<IT,NT> tuples(size, nRow, nCol, mytuples);        
	tuples.SortColBased();
	
#ifdef DEBUG
	pair<IT,IT> rlim = tuples.RowLimits(); 
	pair<IT,IT> clim = tuples.ColLimits();

	ofstream oput;
	stringstream ss;
	string rank;
	int myrank;
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	ss << myrank;
	ss >> rank;
	string ofilename = "Read";
	ofilename += rank;
	oput.open(ofilename.c_str(), ios_base::app );
	oput << "Creating of dimensions " << nRow << "-by-" << nCol << " of size: " << size << 
			" with row range (" << rlim.first  << "," << rlim.second << ") and column range (" << clim.first  << "," << clim.second << ")" << endl;
	if(tuples.getnnz() > 0)
	{ 
		IT minfr = joker::get<0>(tuples.front());
		IT minto = joker::get<1>(tuples.front());
		IT maxfr = joker::get<0>(tuples.back());
		IT maxto = joker::get<1>(tuples.back());

		oput << "Min: " << minfr << ", " << minto << "; Max: " << maxfr << ", " << maxto << endl;
	}
	oput.close();
#endif

	SpDCCols<IT,NT> object(tuples, false);	
	*this = object;
}


template <class IT, class NT>
vector<IT> SpDCCols<IT,NT>::GetEssentials() const
{
	vector<IT> essentials(esscount);
	essentials[0] = nnz;
	essentials[1] = m;
	essentials[2] = n;
	essentials[3] = (nnz > 0) ? dcsc->nzc : 0;
	return essentials;
}

template <class IT, class NT>
template <typename NNT>
SpDCCols<IT,NT>::operator SpDCCols<IT,NNT> () const
{
	Dcsc<IT,NNT> * convert;
	if(nnz > 0)
		convert = new Dcsc<IT,NNT>(*dcsc);
	else
		convert = NULL;

	return SpDCCols<IT,NNT>(m, n, convert);
}


template <class IT, class NT>
template <typename NIT, typename NNT>
SpDCCols<IT,NT>::operator SpDCCols<NIT,NNT> () const
{
	Dcsc<NIT,NNT> * convert;
	if(nnz > 0)
		convert = new Dcsc<NIT,NNT>(*dcsc);
	else
		convert = NULL;

	return SpDCCols<NIT,NNT>(m, n, convert);
}


template <class IT, class NT>
Arr<IT,NT> SpDCCols<IT,NT>::GetArrays() const
{
	Arr<IT,NT> arr(3,1);

	if(nnz > 0)
	{
		arr.indarrs[0] = LocArr<IT,IT>(dcsc->cp, dcsc->nzc+1);
		arr.indarrs[1] = LocArr<IT,IT>(dcsc->jc, dcsc->nzc);
		arr.indarrs[2] = LocArr<IT,IT>(dcsc->ir, dcsc->nz);
		arr.numarrs[0] = LocArr<NT,IT>(dcsc->numx, dcsc->nz);
	}
	else
	{
		arr.indarrs[0] = LocArr<IT,IT>(NULL, 0);
		arr.indarrs[1] = LocArr<IT,IT>(NULL, 0);
		arr.indarrs[2] = LocArr<IT,IT>(NULL, 0);
		arr.numarrs[0] = LocArr<NT,IT>(NULL, 0);
	
	}
	return arr;
}

/**
  * O(nnz log(nnz)) time Transpose function
  * \remarks Performs a lexicographical sort
  * \remarks Mutator function (replaces the calling object with its transpose)
  */
template <class IT, class NT>
void SpDCCols<IT,NT>::Transpose()
{
	if(nnz > 0)
	{
		SpTuples<IT,NT> Atuples(*this);
		Atuples.SortRowBased();

		// destruction of (*this) is handled by the assignment operator
		*this = SpDCCols<IT,NT>(Atuples,true);
	}
	else
	{
		*this = SpDCCols<IT,NT>(0, n, m, 0);
	}
}


/**
  * O(nnz log(nnz)) time Transpose function
  * \remarks Performs a lexicographical sort
  * \remarks Const function (doesn't mutate the calling object)
  */
template <class IT, class NT>
SpDCCols<IT,NT> SpDCCols<IT,NT>::TransposeConst() const
{
	SpTuples<IT,NT> Atuples(*this);
	Atuples.SortRowBased();

	return SpDCCols<IT,NT>(Atuples,true);
}

/** 
  * Splits the matrix into two parts, simply by cutting along the columns
  * Simple algorithm that doesn't intend to split perfectly, but it should do a pretty good job
  * Practically destructs the calling object also (frees most of its memory)
  */
template <class IT, class NT>
void SpDCCols<IT,NT>::Split(SpDCCols<IT,NT> & partA, SpDCCols<IT,NT> & partB) 
{
	IT cut = n/2;
	if(cut == 0)
	{
		cout<< "Matrix is too small to be splitted" << endl;
		return;
	}

	Dcsc<IT,NT> *Adcsc = NULL;
	Dcsc<IT,NT> *Bdcsc = NULL;

	if(nnz != 0)
	{
		dcsc->Split(Adcsc, Bdcsc, cut);
	}

	partA = SpDCCols<IT,NT> (m, cut, Adcsc);
	partB = SpDCCols<IT,NT> (m, n-cut, Bdcsc);
	
	// handle destruction through assignment operator
	*this = SpDCCols<IT, NT>();		
}

/** 
  * Merges two matrices (cut along the columns) into 1 piece
  * Split method should have been executed on the object beforehand
 **/
template <class IT, class NT>
void SpDCCols<IT,NT>::Merge(SpDCCols<IT,NT> & partA, SpDCCols<IT,NT> & partB) 
{
	assert( partA.m == partB.m );

	Dcsc<IT,NT> * Cdcsc = new Dcsc<IT,NT>();

	if(partA.nnz == 0 && partB.nnz == 0)
	{
		Cdcsc = NULL;
	}
	else if(partA.nnz == 0)
	{
		Cdcsc = new Dcsc<IT,NT>(*(partB.dcsc));
		transform(Cdcsc->jc, Cdcsc->jc + Cdcsc->nzc, Cdcsc->jc, bind2nd(plus<IT>(), partA.n));
	}
	else if(partB.nnz == 0)
	{
		Cdcsc = new Dcsc<IT,NT>(*(partA.dcsc));
	}
	else
	{
		Cdcsc->Merge(partA.dcsc, partB.dcsc, partA.n);
	}
	*this = SpDCCols<IT,NT> (partA.m, partA.n + partB.n, Cdcsc);

	partA = SpDCCols<IT, NT>();	
	partB = SpDCCols<IT, NT>();
}

/**
 * C += A*B' (Using OuterProduct Algorithm)
 * This version is currently limited to multiplication of matrices with the same precision 
 * (e.g. it can't multiply double-precision matrices with booleans)
 * The multiplication is on the specified semiring (passed as parameter)
 */
template <class IT, class NT>
template <class SR>
int SpDCCols<IT,NT>::PlusEq_AnXBt(const SpDCCols<IT,NT> & A, const SpDCCols<IT,NT> & B)
{
	if(A.isZero() || B.isZero())
	{
		return -1;	// no need to do anything
	}
	Isect<IT> *isect1, *isect2, *itr1, *itr2, *cols, *rows;
	SpHelper::SpIntersect(*(A.dcsc), *(B.dcsc), cols, rows, isect1, isect2, itr1, itr2);
	
	IT kisect = static_cast<IT>(itr1-isect1);		// size of the intersection ((itr1-isect1) == (itr2-isect2))
	if(kisect == 0)
	{
		DeleteAll(isect1, isect2, cols, rows);
		return -1;
	}
	
	StackEntry< NT, pair<IT,IT> > * multstack;
	IT cnz = SpHelper::SpCartesian< SR > (*(A.dcsc), *(B.dcsc), kisect, isect1, isect2, multstack);  
	DeleteAll(isect1, isect2, cols, rows);

	IT mdim = A.m;	
	IT ndim = B.m;		// since B has already been transposed
	if(isZero())
	{
		dcsc = new Dcsc<IT,NT>(multstack, mdim, ndim, cnz);
	}
	else
	{
		dcsc->AddAndAssign(multstack, mdim, ndim, cnz);
	}
	nnz = dcsc->nz;

	delete [] multstack;
	return 1;	
}

/**
 * C += A*B (Using ColByCol Algorithm)
 * This version is currently limited to multiplication of matrices with the same precision 
 * (e.g. it can't multiply double-precision matrices with booleans)
 * The multiplication is on the specified semiring (passed as parameter)
 */
template <class IT, class NT>
template <typename SR>
int SpDCCols<IT,NT>::PlusEq_AnXBn(const SpDCCols<IT,NT> & A, const SpDCCols<IT,NT> & B)
{
	if(A.isZero() || B.isZero())
	{
		return -1;	// no need to do anything
	}
	StackEntry< NT, pair<IT,IT> > * multstack;
	int cnz = SpHelper::SpColByCol< SR > (*(A.dcsc), *(B.dcsc), A.n, multstack);  
	
	IT mdim = A.m;	
	IT ndim = B.n;
	if(isZero())
	{
		dcsc = new Dcsc<IT,NT>(multstack, mdim, ndim, cnz);
	}
	else
	{
		dcsc->AddAndAssign(multstack, mdim, ndim, cnz);
	}
	nnz = dcsc->nz;
	
	delete [] multstack;
	return 1;	
}


template <class IT, class NT>
template <typename SR>
int SpDCCols<IT,NT>::PlusEq_AtXBn(const SpDCCols<IT,NT> & A, const SpDCCols<IT,NT> & B)
{
	cout << "PlusEq_AtXBn function has not been implemented yet !" << endl;
	return 0;
}

template <class IT, class NT>
template <typename SR>
int SpDCCols<IT,NT>::PlusEq_AtXBt(const SpDCCols<IT,NT> & A, const SpDCCols<IT,NT> & B)
{
	cout << "PlusEq_AtXBt function has not been implemented yet !" << endl;
	return 0;
}


template <class IT, class NT>
SpDCCols<IT,NT> SpDCCols<IT,NT>::operator() (IT ri, IT ci) const
{
	IT * itr = find(dcsc->jc, dcsc->jc + dcsc->nzc, ci);
	if(itr != dcsc->jc + dcsc->nzc)
	{
		IT irbeg = dcsc->cp[itr - dcsc->jc];
		IT irend = dcsc->cp[itr - dcsc->jc + 1];

		IT * ele = find(dcsc->ir + irbeg, dcsc->ir + irend, ri);
		if(ele != dcsc->ir + irend)	
		{	
			SpDCCols<IT,NT> SingEleMat(1, 1, 1, 1);	// 1-by-1 matrix with 1 nonzero 
			*(SingEleMat.dcsc->numx) = dcsc->numx[ele - dcsc->ir];
			*(SingEleMat.dcsc->ir) = *ele; 
			*(SingEleMat.dcsc->jc) = *itr;
			(SingEleMat.dcsc->cp)[0] = 0;
			(SingEleMat.dcsc->cp)[1] = 1;
		}
		else
		{
			return SpDCCols<IT,NT>();  // 0-by-0 empty matrix
		}
	}
	else
	{
		return SpDCCols<IT,NT>();	 // 0-by-0 empty matrix		
	}
}

/** 
 * The almighty indexing polyalgorithm 
 * Calls different subroutines depending the sparseness of ri/ci
 */
template <class IT, class NT>
SpDCCols<IT,NT> SpDCCols<IT,NT>::operator() (const vector<IT> & ri, const vector<IT> & ci) const
{
	typedef PlusTimesSRing<NT,NT> PT;	

	IT rsize = ri.size();
	IT csize = ci.size();

	if(rsize == 0 && csize == 0)
	{
		// return an m x n matrix of complete zeros
		// since we don't know whether columns or rows are indexed
		return SpDCCols<IT,NT> (0, m, n, 0);		
	}
	else if(rsize == 0)
	{
		return ColIndex(ci);
	}
	else if(csize == 0)
	{
		SpDCCols<IT,NT> LeftMatrix(rsize, rsize, this->m, ri, true);
		return LeftMatrix.OrdColByCol< PT >(*this);
	}
	else	// this handles the (rsize=1 && csize=1) case as well
	{
		SpDCCols<IT,NT> LeftMatrix(rsize, rsize, this->m, ri, true);
		SpDCCols<IT,NT> RightMatrix(csize, this->n, csize, ci, false);
		return LeftMatrix.OrdColByCol< PT >( OrdColByCol< PT >(RightMatrix) );
	}
}

template <class IT, class NT>
ofstream & SpDCCols<IT,NT>::put(ofstream & outfile) const 
{
	if(nnz == 0)
	{
		outfile << "Matrix doesn't have any nonzeros" <<endl;
		return outfile;
	}
	SpTuples<IT,NT> tuples(*this); 
	outfile << tuples << endl;
	return outfile;
}


template <class IT, class NT>
ifstream & SpDCCols<IT,NT>::get(ifstream & infile)
{
	cout << "Getting... SpDCCols" << endl;
	IT m, n, nnz;
	infile >> m >> n >> nnz;
	SpTuples<IT,NT> tuples(nnz, m, n);        
	infile >> tuples;
	tuples.SortColBased();
        
	SpDCCols<IT,NT> object(tuples, false, NULL);	
	*this = object;
	return infile;
}


template<class IT, class NT>
void SpDCCols<IT,NT>::PrintInfo(ofstream &  out) const
{
	out << "m: " << m ;
	out << ", n: " << n ;
	out << ", nnz: "<< nnz ;

	if(splits > 0)
	{
		out << ", local splits: " << splits << endl;
	}
	else
	{
		if(dcsc != NULL)
		{
			out << ", nzc: "<< dcsc->nzc << endl;
		}
		else
		{
			out <<", nzc: "<< 0 << endl;
		}
	}
}

template<class IT, class NT>
void SpDCCols<IT,NT>::PrintInfo() const
{
	cout << "m: " << m ;
	cout << ", n: " << n ;
	cout << ", nnz: "<< nnz ;

	if(splits > 0)
	{
		cout << ", local splits: " << splits << endl;
	}
	else
	{
		if(dcsc != NULL)
		{
			cout << ", nzc: "<< dcsc->nzc << endl;
		}
		else
		{
			cout <<", nzc: "<< 0 << endl;
		}

		if(m < PRINT_LIMIT && n < PRINT_LIMIT)	// small enough to print
		{
			NT ** A = SpHelper::allocate2D<NT>(m,n);
			for(IT i=0; i< m; ++i)
				for(IT j=0; j<n; ++j)
					A[i][j] = NT();
			if(dcsc != NULL)
			{
				for(IT i=0; i< dcsc->nzc; ++i)
				{
					for(IT j = dcsc->cp[i]; j<dcsc->cp[i+1]; ++j)
					{
						IT colid = dcsc->jc[i];
						IT rowid = dcsc->ir[j];
						A[rowid][colid] = dcsc->numx[j];
					}
				}
			} 
			for(IT i=0; i< m; ++i)
			{
				for(IT j=0; j<n; ++j)
				{
					cout << setiosflags(ios::fixed) << setprecision(2) << A[i][j];
					cout << " ";
				}
				cout << endl;
			}
			SpHelper::deallocate2D(A,m);
		}
	}
}


/****************************************************************************/
/********************* PRIVATE CONSTRUCTORS/DESTRUCTORS *********************/
/****************************************************************************/

//! Construct SpDCCols from Dcsc
template <class IT, class NT>
SpDCCols<IT,NT>::SpDCCols(IT nRow, IT nCol, Dcsc<IT,NT> * mydcsc)
:dcsc(mydcsc), m(nRow), n(nCol), splits(0)
{
	if (mydcsc == NULL) 
		nnz = 0;
	else
		nnz = mydcsc->nz;
}

//! Create a logical matrix from (row/column) indices array, used for indexing only
template <class IT, class NT>
SpDCCols<IT,NT>::SpDCCols (IT size, IT nRow, IT nCol, const vector<IT> & indices, bool isRow)
:m(nRow), n(nCol), nnz(size), splits(0)
{
	if(size > 0)
		dcsc = new Dcsc<IT,NT>(size,indices,isRow);
	else
		dcsc = NULL; 
}


/****************************************************************************/
/************************* PRIVATE MEMBER FUNCTIONS *************************/
/****************************************************************************/

template <class IT, class NT>
inline void SpDCCols<IT,NT>::CopyDcsc(Dcsc<IT,NT> * source)
{
	// source dcsc will be NULL if number of nonzeros is zero 
	if(source != NULL)	
		dcsc = new Dcsc<IT,NT>(*source);
	else
		dcsc = NULL;
}

/**
 * \return An indexed SpDCCols object without using multiplication 
 * \pre ci is sorted and is not completely empty.
 * \remarks it is OK for some indices ci[i] to be empty in the indexed SpDCCols matrix 
 *	[i.e. in the output, nzc does not need to be equal to n]
 */
template <class IT, class NT>
SpDCCols<IT,NT> SpDCCols<IT,NT>::ColIndex(const vector<IT> & ci) const
{
	IT csize = ci.size();
	if(nnz == 0)	// nothing to index
	{
		return SpDCCols<IT,NT>(0, m, csize, 0);	
	}
	else if(ci.empty())
	{
		return SpDCCols<IT,NT>(0, m,0, 0);
	}

	// First pass for estimation
	IT estsize = 0;
	IT estnzc = 0;
	for(IT i=0, j=0;  i< dcsc->nzc && j < csize;)
	{
		if((dcsc->jc)[i] < ci[j])
		{
			++i;
		}
		else if ((dcsc->jc)[i] > ci[j])
		{
			++j;
		}
		else
		{
			estsize +=  (dcsc->cp)[i+1] - (dcsc->cp)[i];
			++estnzc;
			++i;
			++j;
		}
	}
	
	SpDCCols<IT,NT> SubA(estsize, m, csize, estnzc);	
	if(estnzc == 0)
	{
		return SubA;		// no need to run the second pass
	}
	SubA.dcsc->cp[0] = 0;
	IT cnzc = 0;
	IT cnz = 0;
	for(IT i=0, j=0;  i < dcsc->nzc && j < csize;)
	{
		if((dcsc->jc)[i] < ci[j])
		{
			++i;
		}
		else if ((dcsc->jc)[i] > ci[j])		// an empty column for the output
		{
			++j;
		}
		else
		{
			IT columncount = (dcsc->cp)[i+1] - (dcsc->cp)[i];
			SubA.dcsc->jc[cnzc++] = j;
			SubA.dcsc->cp[cnzc] = SubA.dcsc->cp[cnzc-1] + columncount;
			copy(dcsc->ir + dcsc->cp[i], dcsc->ir + dcsc->cp[i+1], SubA.dcsc->ir + cnz);
			copy(dcsc->numx + dcsc->cp[i], dcsc->numx + dcsc->cp[i+1], SubA.dcsc->numx + cnz);
			cnz += columncount;
			++i;
			++j;
		}
	}
	return SubA;
}

template <class IT, class NT>
template <typename SR, typename NTR>
SpDCCols< IT, typename promote_trait<NT,NTR>::T_promote > SpDCCols<IT,NT>::OrdOutProdMult(const SpDCCols<IT,NTR> & rhs) const
{
	typedef typename promote_trait<NT,NTR>::T_promote T_promote;  

	if(isZero() || rhs.isZero())
	{
		return SpDCCols< IT, T_promote > (0, m, rhs.n, 0);		// return an empty matrix	
	}
	SpDCCols<IT,NTR> Btrans = rhs.TransposeConst();

	Isect<IT> *isect1, *isect2, *itr1, *itr2, *cols, *rows;
	SpHelper::SpIntersect(*dcsc, *(Btrans.dcsc), cols, rows, isect1, isect2, itr1, itr2);
	
	IT kisect = static_cast<IT>(itr1-isect1);		// size of the intersection ((itr1-isect1) == (itr2-isect2))
	if(kisect == 0)
	{
		DeleteAll(isect1, isect2, cols, rows);
		return SpDCCols< IT, T_promote > (0, m, rhs.n, 0);	
	}
	StackEntry< T_promote, pair<IT,IT> > * multstack;
	IT cnz = SpHelper::SpCartesian< SR > (*dcsc, *(Btrans.dcsc), kisect, isect1, isect2, multstack);  
	DeleteAll(isect1, isect2, cols, rows);

	Dcsc<IT, T_promote> * mydcsc = NULL;
	if(cnz > 0)
	{
		mydcsc = new Dcsc< IT,T_promote > (multstack, m, rhs.n, cnz);
		delete [] multstack;
	}
	return SpDCCols< IT,T_promote > (m, rhs.n, mydcsc);	
}


template <class IT, class NT>
template <typename SR, typename NTR>
SpDCCols< IT, typename promote_trait<NT,NTR>::T_promote > SpDCCols<IT,NT>::OrdColByCol(const SpDCCols<IT,NTR> & rhs) const
{
	typedef typename promote_trait<NT,NTR>::T_promote T_promote;  

	if(isZero() || rhs.isZero())
	{
		return SpDCCols<IT, T_promote> (0, m, rhs.n, 0);		// return an empty matrix	
	}
	StackEntry< T_promote, pair<IT,IT> > * multstack;
	IT cnz = SpHelper::SpColByCol< SR > (*dcsc, *(rhs.dcsc), n, multstack);  
	
	Dcsc<IT,T_promote > * mydcsc = NULL;
	if(cnz > 0)
	{
		mydcsc = new Dcsc< IT,T_promote > (multstack, m, rhs.n, cnz);
		delete [] multstack;
	}
	return SpDCCols< IT,T_promote > (m, rhs.n, mydcsc);	
}

