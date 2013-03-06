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

#include "dcsc.h"
#include <algorithm>
#include <functional>
#include <iostream>
#include "Friends.h"
#include "SpHelper.h"
using namespace std;

template <class IT, class NT>
Dcsc<IT,NT>::Dcsc ():cp(NULL), jc(NULL), ir(NULL), numx(NULL),nz(0), nzc(0){}

template <class IT, class NT>
Dcsc<IT,NT>::Dcsc (IT nnz, IT nzcol): nz(nnz),nzc(nzcol)
{
	assert (nz != 0);
	assert (nzc != 0);
	cp = new IT[nzc+1];
	jc = new IT[nzc];
	ir = new IT[nz];
	numx = new NT[nz];
}

//! GetIndices helper function for StackEntry arrays
template <class IT, class NT>
inline void Dcsc<IT,NT>::getindices (StackEntry<NT, pair<IT,IT> > * multstack, IT & rindex, IT & cindex, IT & j, IT nnz)
{
	if(j<nnz)
	{
		cindex = multstack[j].key.first;
		rindex = multstack[j].key.second;
	}
	else
	{
		rindex = numeric_limits<IT>::max();
		cindex = numeric_limits<IT>::max();
	}
	++j;
}
	
template <class IT, class NT>
Dcsc<IT,NT> & Dcsc<IT,NT>::AddAndAssign (StackEntry<NT, pair<IT,IT> > * multstack, IT mdim, IT ndim, IT nnz)
{
	if(nnz == 0)	return *this;
		
	IT estnzc = nzc + nnz;
	IT estnz  = nz + nnz;
	Dcsc<IT,NT> temp(estnz, estnzc);

	IT curnzc = 0;		// number of nonzero columns constructed so far
	IT curnz = 0;
	IT i = 0;
	IT j = 0;
	IT rindex, cindex;
	getindices(multstack, rindex, cindex,j,nnz);

	temp.cp[0] = 0;
	while(i< nzc && cindex < numeric_limits<IT>::max())	// i runs over columns of "this",  j runs over all the nonzeros of "multstack"
	{
		if(jc[i] > cindex)
		{
			IT columncount = 0;
			temp.jc[curnzc++] = cindex;
			do
			{
				temp.ir[curnz] 		= rindex;
				temp.numx[curnz++] 	= multstack[j-1].value;

				getindices(multstack, rindex, cindex,j,nnz);
				++columncount;
			}
			while(temp.jc[curnzc-1] == cindex);	// loop until cindex changes

			temp.cp[curnzc] = temp.cp[curnzc-1] + columncount;
		}
		else if(jc[i] < cindex)
		{
			temp.jc[curnzc++] = jc[i++];
			for(IT k = cp[i-1]; k< cp[i]; ++k)
			{
				temp.ir[curnz] 		= ir[k];
				temp.numx[curnz++] 	= numx[k];
			}
			temp.cp[curnzc] = temp.cp[curnzc-1] + (cp[i] - cp[i-1]);
		}
		else	// they are equal, merge the column
		{
			temp.jc[curnzc++] = jc[i];
			IT ii = cp[i];
			IT prevnz = curnz;		
			while (ii < cp[i+1] && cindex == jc[i])	// cindex would be MAX if multstack is deplated
			{
				if (ir[ii] < rindex)
				{
					temp.ir[curnz] = ir[ii];
					temp.numx[curnz++] = numx[ii++];
				}
				else if (ir[ii] > rindex)
				{
					temp.ir[curnz] = rindex;
					temp.numx[curnz++] = multstack[j-1].value;

					getindices(multstack, rindex, cindex,j,nnz);
				}
				else
				{
					temp.ir[curnz] = ir[ii];
					temp.numx[curnz++] = numx[ii++] + multstack[j-1].value;

					getindices(multstack, rindex, cindex,j,nnz);
				}
			}
			while (ii < cp[i+1])
			{
				temp.ir[curnz] = ir[ii];
				temp.numx[curnz++] = numx[ii++];
			}
			while (cindex == jc[i])
			{
				temp.ir[curnz] = rindex;
				temp.numx[curnz++] = multstack[j-1].value;

				getindices(multstack, rindex, cindex,j,nnz);
			}
			temp.cp[curnzc] = temp.cp[curnzc-1] + curnz-prevnz;
			++i;
		}
	}
	while(i< nzc)
	{
		temp.jc[curnzc++] = jc[i++];
		for(IT k = cp[i-1]; k< cp[i]; ++k)
		{
			temp.ir[curnz] 		= ir[k];
			temp.numx[curnz++] 	= numx[k];
		}
		temp.cp[curnzc] = temp.cp[curnzc-1] + (cp[i] - cp[i-1]);
	}
	while(cindex < numeric_limits<IT>::max())
	{
		IT columncount = 0;
		temp.jc[curnzc++] = cindex;
		do
		{
			temp.ir[curnz] 		= rindex;
			temp.numx[curnz++] 	= multstack[j-1].value;

			getindices(multstack, rindex, cindex,j,nnz);
			++columncount;
		}
		while(temp.jc[curnzc-1] == cindex);	// loop until cindex changes

		temp.cp[curnzc] = temp.cp[curnzc-1] + columncount;
	}
	temp.Resize(curnzc, curnz);
	*this = temp;
	return *this;
}


/**
  * Creates DCSC structure from an array of StackEntry's
  * \remark Complexity: O(nnz)
  */
template <class IT, class NT>
Dcsc<IT,NT>::Dcsc (StackEntry<NT, pair<IT,IT> > * multstack, IT mdim, IT ndim, IT nnz): nz(nnz)
{
	nzc = std::min(ndim, nnz);	// nzc can't exceed any of those

	assert(nz != 0 );
	cp = new IT[nzc+1];	// to be shrinked
	jc = new IT[nzc];	// to be shrinked
	ir = new IT[nz];
	numx = new NT[nz];
	
	IT curnzc = 0;				// number of nonzero columns constructed so far
	IT cindex = multstack[0].key.first;
	IT rindex = multstack[0].key.second;

	ir[0]	= rindex;
	numx[0] = multstack[0].value;
	jc[curnzc] = cindex;
	cp[curnzc] = 0; 
	++curnzc;

	for(IT i=1; i<nz; ++i)
	{
		cindex = multstack[i].key.first;
		rindex = multstack[i].key.second;

		ir[i]	= rindex;
		numx[i] = multstack[i].value;
		if(cindex != jc[curnzc-1])
		{
			jc[curnzc] = cindex;
			cp[curnzc++] = i;
		}
	}
	cp[curnzc] = nz;
	Resize(curnzc, nz);	// only shrink cp & jc arrays
}

/**
  * Create a logical matrix from (row/column) indices array
  * \remark This function should only be used for indexing 
  * \remark For these temporary matrices nz = nzc (which are both equal to nnz)
  */
template <class IT, class NT>
Dcsc<IT,NT>::Dcsc (IT nnz, const vector<IT> & indices, bool isRow): nz(nnz),nzc(nnz)
{
	assert((nnz != 0) && (indices.size() == nnz));
	cp = new IT[nnz+1];	
	jc = new IT[nnz];
	ir = new IT[nnz];
	numx = new NT[nnz];

	SpHelper::iota(cp, cp+nnz+1, 0);  // insert sequential values {0,1,2,..}
	fill_n(numx, nnz, static_cast<NT>(1));
	
	if(isRow)
	{
		SpHelper::iota(ir, ir+nnz, 0);	
		std::copy (indices.begin(), indices.end(), jc);
	}
	else
	{
		SpHelper::iota(jc, jc+nnz, 0);
		std::copy (indices.begin(), indices.end(), ir);	
	}
}


template <class IT, class NT>
template <typename NNT>
Dcsc<IT,NT>::operator Dcsc<IT,NNT>() const
{
	Dcsc<IT,NNT> convert(nz, nzc);	
	
	for(IT i=0; i< nz; ++i)
	{		
		convert.numx[i] = static_cast<NNT>(numx[i]);		
	}
	std::copy(ir, ir+nz, convert.ir);	// copy(first, last, result)
	std::copy(jc, jc+nzc, convert.jc);
	std::copy(cp, cp+nzc+1, convert.cp);
	return convert;
}

template <class IT, class NT>
template <typename NIT, typename NNT>
Dcsc<IT,NT>::operator Dcsc<NIT,NNT>() const
{
	Dcsc<NIT,NNT> convert(nz, nzc);	
	
	for(IT i=0; i< nz; ++i)
		convert.numx[i] = static_cast<NNT>(numx[i]);		
	for(IT i=0; i< nz; ++i)
		convert.ir[i] = static_cast<NIT>(ir[i]);
	for(IT i=0; i< nzc; ++i)
		convert.jc[i] = static_cast<NIT>(jc[i]);
	for(IT i=0; i<= nzc; ++i)
		convert.cp[i] = static_cast<NIT>(cp[i]);
	return convert;
}

template <class IT, class NT>
Dcsc<IT,NT>::Dcsc (const Dcsc<IT,NT> & rhs): nz(rhs.nz), nzc(rhs.nzc)
{
	if(nz > 0)
	{
		numx = new NT[nz];
		ir = new IT[nz];
		std::copy(rhs.numx, rhs.numx + nz, numx);	// numx can be a non-POD type
		std::copy(rhs.ir, rhs.ir + nz, ir);
	}
	else
	{
		numx = NULL;
		ir = NULL;
	}
	if(nzc > 0)
	{
		jc = new IT[nzc];
		cp = new IT[nzc+1];
		std::copy(rhs.jc, rhs.jc + nzc, jc);
		std::copy(rhs.cp, rhs.cp + nzc + 1, cp);
	}
	else
	{
		jc = NULL;
		cp = NULL;
	}
}

/**
  * Assignment operator (called on an existing object)
  */
template <class IT, class NT>
Dcsc<IT,NT> & Dcsc<IT,NT>::operator=(const Dcsc<IT,NT> & rhs)
{
	if(this != &rhs)		
	{
		// make empty first !
		if(nz > 0)
		{
			delete[] numx;
			delete[] ir;	
		}
		if(nzc > 0)
		{
			delete[] jc;
			delete[] cp;
		}
		nz = rhs.nz;
		nzc = rhs.nzc;
		if(nz > 0)
		{
			numx = new NT[nz];
			ir = new IT[nz];
			std::copy(rhs.numx, rhs.numx + nz, numx);	// numx can be a non-POD type
			std::copy(rhs.ir, rhs.ir + nz, ir);
		}
		else
		{
			numx = NULL;
			ir = NULL;
		}
		if(nzc > 0)
		{
			jc = new IT[nzc];
	                cp = new IT[nzc+1];
        	        std::copy(rhs.jc, rhs.jc + nzc, jc);
                	std::copy(rhs.cp, rhs.cp + nzc + 1, cp);
		}
		else
		{
			jc = NULL;
			cp = NULL;
		}
	}
	return *this;
}

template <class IT, class NT>
Dcsc<IT, NT> & Dcsc<IT,NT>::operator+=(const Dcsc<IT,NT> & rhs)	// add and assign operator
{
	IT estnzc = nzc + rhs.nzc;
	IT estnz  = nz + rhs.nz;
	Dcsc<IT,NT> temp(estnz, estnzc);

	IT curnzc = 0;
	IT curnz = 0;
	IT i = 0;
	IT j = 0;
	temp.cp[0] = 0;
	while(i< nzc && j<rhs.nzc)
	{
		if(jc[i] > rhs.jc[j])
		{
			temp.jc[curnzc++] = rhs.jc[j++];
			for(IT k = rhs.cp[j-1]; k< rhs.cp[j]; ++k)
			{
				temp.ir[curnz] 		= rhs.ir[k];
				temp.numx[curnz++] 	= rhs.numx[k];
			}
			temp.cp[curnzc] = temp.cp[curnzc-1] + (rhs.cp[j] - rhs.cp[j-1]);
		}
		else if(jc[i] < rhs.jc[j])
		{
			temp.jc[curnzc++] = jc[i++];
			for(IT k = cp[i-1]; k< cp[i]; k++)
			{
				temp.ir[curnz] 		= ir[k];
				temp.numx[curnz++] 	= numx[k];
			}
			temp.cp[curnzc] = temp.cp[curnzc-1] + (cp[i] - cp[i-1]);
		}
		else
		{
			temp.jc[curnzc++] = jc[i];
			IT ii = cp[i];
			IT jj = rhs.cp[j];
			IT prevnz = curnz;		
			while (ii < cp[i+1] && jj < rhs.cp[j+1])
			{
				if (ir[ii] < rhs.ir[jj])
				{
					temp.ir[curnz] = ir[ii];
					temp.numx[curnz++] = numx[ii++];
				}
				else if (ir[ii] > rhs.ir[jj])
				{
					temp.ir[curnz] = rhs.ir[jj];
					temp.numx[curnz++] = rhs.numx[jj++];
				}
				else
				{
					temp.ir[curnz] = ir[ii];
					temp.numx[curnz++] = numx[ii++] + rhs.numx[jj++];	// might include zeros
				}
			}
			while (ii < cp[i+1])
			{
				temp.ir[curnz] = ir[ii];
				temp.numx[curnz++] = numx[ii++];
			}
			while (jj < rhs.cp[j+1])
			{
				temp.ir[curnz] = rhs.ir[jj];
				temp.numx[curnz++] = rhs.numx[jj++];
			}
			temp.cp[curnzc] = temp.cp[curnzc-1] + curnz-prevnz;
			++i;
			++j;
		}
	}
	while(i< nzc)
	{
		temp.jc[curnzc++] = jc[i++];
		for(IT k = cp[i-1]; k< cp[i]; ++k)
		{
			temp.ir[curnz] 	= ir[k];
			temp.numx[curnz++] = numx[k];
		}
		temp.cp[curnzc] = temp.cp[curnzc-1] + (cp[i] - cp[i-1]);
	}
	while(j < rhs.nzc)
	{
		temp.jc[curnzc++] = rhs.jc[j++];
		for(IT k = rhs.cp[j-1]; k< rhs.cp[j]; ++k)
		{
			temp.ir[curnz] 	= rhs.ir[k];
			temp.numx[curnz++] 	= rhs.numx[k];
		}
		temp.cp[curnzc] = temp.cp[curnzc-1] + (rhs.cp[j] - rhs.cp[j-1]);
	}
	temp.Resize(curnzc, curnz);
	*this = temp;
	return *this;
}

template <class IT, class NT>
bool Dcsc<IT,NT>::operator==(const Dcsc<IT,NT> & rhs)
{
	if(nzc != rhs.nzc) return false;
	bool same = std::equal(cp, cp+nzc+1, rhs.cp); 
	same = same && std::equal(jc, jc+nzc, rhs.jc);
	same = same && std::equal(ir, ir+nz, rhs.ir);
	
#ifdef DEBUG
	vector<NT> error(nz);
	transform(numx, numx+nz, rhs.numx, error.begin(), absdiff<NT>());
	vector< pair<NT, NT> > error_original_pair(nz);
	for(IT i=0; i < nz; ++i)
		error_original_pair[i] = make_pair(error[i], numx[i]);
	if(error_original_pair.size() > 10)	// otherwise would crush for small data
	{
		partial_sort(error_original_pair.begin(), error_original_pair.begin()+10, error_original_pair.end(), greater< pair<NT,NT> >());
		cout << "Highest 10 different entries are: " << endl;
		for(IT i=0; i < 10; ++i)
			cout << "Diff: " << error_original_pair[i].first << " on " << error_original_pair[i].second << endl;
	}
	else
	{
		sort(error_original_pair.begin(), error_original_pair.end(), greater< pair<NT,NT> >());
		cout << "Highest different entries are: " << endl;
		for(typename vector< pair<NT, NT> >::iterator it=error_original_pair.begin(); it != error_original_pair.end(); ++it)
			cout << "Diff: " << it->first << " on " << it->second << endl;
	}
	cout << "Same before num: " << same << endl;
#endif

	ErrorTolerantEqual<NT> epsilonequal(EPSILON);
	same = same && std::equal(numx, numx+nz, rhs.numx, epsilonequal );
	return same;
}

/**
 * @param[in]   exclude if false,
 *      \n              then operation is A = A .* B
 *      \n              else operation is A = A .* not(B) 
 **/
template <class IT, class NT>
void Dcsc<IT,NT>::EWiseMult(const Dcsc<IT,NT> & rhs, bool exclude)	
{
	// We have a class with a friend function and a member function with the same name. Calling the friend function from the member function 
	// might (if the signature is the same) give compilation errors if not preceded by :: that denotes the global scope.
	*this = ::EWiseMult((*this), &rhs, exclude);	// call the binary version
}


template <class IT, class NT>
template <typename _UnaryOperation>
Dcsc<IT,NT>* Dcsc<IT,NT>::Prune(_UnaryOperation __unary_op, bool inPlace)
{
	// Two-pass algorithm
	IT prunednnz = 0;
	IT prunednzc = 0;
	for(IT i=0; i<nzc; ++i)
	{
		bool colexists = false;
		for(IT j=cp[i]; j < cp[i+1]; ++j)
		{
			if(!(__unary_op(numx[j]))) 	// keep this nonzero
			{
				++prunednnz;
				colexists = true;
			}
		}
		if(colexists) 	++prunednzc;
	}
	IT * oldcp = cp; 
	IT * oldjc = jc;
	IT * oldir = ir;	
	NT * oldnumx = numx;	

	cp = new IT[prunednzc+1];
	jc = new IT[prunednzc];
	ir = new IT[prunednnz];
	numx = new NT[prunednnz];

	IT cnzc = 0;
	IT cnnz = 0;
	cp[cnzc] = 0;
	for(IT i=0; i<nzc; ++i)
	{
		for(IT j = oldcp[i]; j < oldcp[i+1]; ++j)
		{
			if(!(__unary_op(oldnumx[j]))) // keep this nonzero
			{
				ir[cnnz] = oldir[j];	
				numx[cnnz++] = 	oldnumx[j];
			}
		}
		if(cnnz > cp[cnzc])
		{
			jc[cnzc] = oldjc[i];
			cp[cnzc+1] = cnnz;
			++cnzc;
		}
	}
	assert(cnzc == prunednzc);
	assert(cnnz == prunednnz);
	if (inPlace)
	{
		// delete the memory pointed by previous pointers
		DeleteAll(oldnumx, oldir, oldjc, oldcp);
		nz = cnnz;
		nzc = cnzc;
		return NULL;
	}
	else
	{
		// create a new object to store the data
		Dcsc<IT,NT>* ret = new Dcsc<IT,NT>();
		ret->cp = cp; 
		ret->jc = jc;
		ret->ir = ir;	
		ret->numx = numx;
		ret->nz = cnnz;
		ret->nzc = cnzc;

		// put the previous pointers back		
		cp = oldcp;
		jc = oldjc;
		ir = oldir;
		numx = oldnumx;
		
		return ret;
	}
}

template <class IT, class NT>
void Dcsc<IT,NT>::EWiseScale(NT ** scaler)	
{
	for(IT i=0; i<nzc; ++i)
	{
		IT colid = jc[i];
		for(IT j=cp[i]; j < cp[i+1]; ++j)
		{
			IT rowid = ir[j];
			numx[j] *= scaler[rowid][colid];
		}
	}
}

/**
  * Updates entries of 2D dense array using __binary_op and entries of "this"
  * @pre { __binary_op is a commutative operation}
  */
template <class IT, class NT>
template <typename _BinaryOperation>
void Dcsc<IT,NT>::UpdateDense(NT ** array, _BinaryOperation __binary_op) const
{
	for(IT i=0; i<nzc; ++i)
	{
		IT colid = jc[i];
		for(IT j=cp[i]; j < cp[i+1]; ++j)
		{
			IT rowid = ir[j];
			array[rowid][colid] = __binary_op(array[rowid][colid], numx[j]);
		}
	}
}

/** 
  * Construct an index array called aux
  * Return the size of the contructed array
  * Complexity O(nzc)
 **/ 
template <class IT, class NT>
IT Dcsc<IT,NT>::ConstructAux(IT ndim, IT * & aux) const
{
	float cf  = static_cast<float>(ndim+1) / static_cast<float>(nzc);
	IT colchunks = static_cast<IT> ( ceil( static_cast<float>(ndim+1) / ceil(cf)) );

	aux  = new IT[colchunks+1]; 

	IT chunksize	= static_cast<IT>(ceil(cf));
	IT reg		= 0;
	IT curchunk	= 0;
	aux[curchunk++] = 0;
	for(IT i = 0; i< nzc; ++i)
	{
		if(jc[i] >= curchunk * chunksize)		// beginning of the next chunk
		{
			while(jc[i] >= curchunk * chunksize)	// consider any empty chunks
			{
				aux[curchunk++] = reg;
			}
		}
		reg = i+1;
	}
	while(curchunk <= colchunks)
	{
		aux[curchunk++] = reg;
	}
	return colchunks;
}

/**
  * Resizes cp & jc arrays to nzcnew, ir & numx arrays to nznew
  * Zero overhead in case sizes stay the same 
 **/ 
template <class IT, class NT>
void Dcsc<IT,NT>::Resize(IT nzcnew, IT nznew)
{
	if(nzcnew == 0)
	{
		delete[] jc;
		delete[] cp;
		nzc = 0;
	}
	if(nznew == 0)
	{
		delete[] ir;
		delete[] numx;
		nz = 0;
	}
	if ( nzcnew == 0 && nznew == 0)
	{
		return;	
	}
	if (nzcnew != nzc)	
	{
		IT * tmpcp = cp; 
		IT * tmpjc = jc;
		cp = new IT[nzcnew+1];
		jc = new IT[nzcnew];	
		if(nzcnew > nzc)	// Grow it (copy all of the old elements)
		{
			std::copy(tmpcp, tmpcp+nzc+1, cp);	// copy(first, end, result)
			std::copy(tmpjc, tmpjc+nzc, jc);
		}
		else		// Shrink it (copy only a portion of the old elements)
		{
			std::copy(tmpcp, tmpcp+nzcnew+1, cp);	
			std::copy(tmpjc, tmpjc+nzcnew, jc);
		}
		delete[] tmpcp;	// delete the memory pointed by previous pointers
		delete[] tmpjc;
		nzc = nzcnew;
	}
	if (nznew != nz)
	{	
		NT * tmpnumx = numx; 
		IT * tmpir = ir;
		numx = new NT[nznew];	
		ir = new IT[nznew];
		if(nznew > nz)	// Grow it (copy all of the old elements)
		{
			std::copy(tmpnumx, tmpnumx+nz, numx);	// numx can be non-POD
			std::copy(tmpir, tmpir+nz, ir);
		}
		else	// Shrink it (copy only a portion of the old elements)
		{
			std::copy(tmpnumx, tmpnumx+nznew, numx);	
			std::copy(tmpir, tmpir+nznew, ir);
		}
		delete[] tmpnumx;	// delete the memory pointed by previous pointers
		delete[] tmpir;
		nz = nznew;
	}
}

/**
  * The first part of the indexing algorithm described in the IPDPS'08 paper
  * @param[IT] colind {Column index to search}
  * Find the column with colind. If it exists, return the position of it. 
  * It it doesn't exist, return value is undefined (implementation specific).
 **/
template<class IT, class NT>
IT Dcsc<IT,NT>::AuxIndex(const IT colind, bool & found, IT * aux, IT csize) const
{
	IT base = static_cast<IT>(floor((float) (colind/csize)));
	IT start = aux[base];
	IT end = aux[base+1];

	IT * itr = find(jc + start, jc + end, colind);
	
	found = (itr != jc + end);
	return (itr-jc);
}

/**
 ** Split along the cut (a column index)
 ** Should work even when one of the splits have no nonzeros at all
 **/
template<class IT, class NT>
void Dcsc<IT,NT>::Split(Dcsc<IT,NT> * & A, Dcsc<IT,NT> * & B, IT cut)
{
	IT * itr = lower_bound(jc, jc+nzc, cut);
	IT pos = itr - jc;

	if(cp[pos] == 0)
	{
		A = NULL;
	}
	else
	{
		A = new Dcsc<IT,NT>(cp[pos], pos);
		std::copy(jc, jc+pos, A->jc);
		std::copy(cp, cp+pos+1, A->cp);
		std::copy(ir, ir+cp[pos], A->ir);
		std::copy(numx, numx + cp[pos], A->numx);	// copy(first, last, result)
	}	
	if(nz-cp[pos] == 0)
	{
		B = NULL;
	}
	else
	{
		B = new Dcsc<IT,NT>(nz-cp[pos], nzc-pos);
		std::copy(jc+pos, jc+ nzc, B->jc);
		transform(B->jc, B->jc + (nzc-pos), B->jc, bind2nd(minus<IT>(), cut));
		std::copy(cp+pos, cp+nzc+1, B->cp);
		transform(B->cp, B->cp + (nzc-pos+1), B->cp, bind2nd(minus<IT>(), cp[pos]));
		std::copy(ir+cp[pos], ir+nz, B->ir);
		std::copy(numx+cp[pos], numx+nz, B->numx);	// copy(first, last, result)
	}
}

// Assumes A and B are not NULL
// When any is NULL, this function is not called anyway
template<class IT, class NT>
void Dcsc<IT,NT>::Merge(const Dcsc<IT,NT> * A, const Dcsc<IT,NT> * B, IT cut)
{
	assert((A != NULL) && (B != NULL)); 	// handled at higher level
	IT cnz = A->nz + B->nz;
	IT cnzc = A->nzc + B->nzc;
	if(cnz > 0)
	{
		*this = Dcsc<IT,NT>(cnz, cnzc);		// safe, because "this" can not be NULL inside a member function

		std::copy(A->jc, A->jc + A->nzc, jc);	// copy(first, last, result)
		std::copy(B->jc, B->jc + B->nzc, jc + A->nzc);
		transform(jc + A->nzc, jc + cnzc, jc + A->nzc, bind2nd(plus<IT>(), cut));

		std::copy(A->cp, A->cp + A->nzc, cp);
		std::copy(B->cp, B->cp + B->nzc +1, cp + A->nzc);
		transform(cp + A->nzc, cp+cnzc+1, cp + A->nzc, bind2nd(plus<IT>(), A->cp[A->nzc]));
	
		std::copy(A->ir, A->ir + A->nz, ir);
		std::copy(B->ir, B->ir + B->nz, ir + A->nz);

		// since numx is potentially non-POD, we use std::copy
		std::copy(A->numx, A->numx + A->nz, numx);
		std::copy(B->numx, B->numx + B->nz, numx + A->nz);
	}
}

/**
 * param[in] nind { length(colsums), gives number of columns of A that contributes to C(:,i) }
 * Vector type VT is allowed to be different than matrix type (IT)
 * However, VT should be up-castable to IT (example: VT=int32_t, IT=int64_t)
 **/
template<class IT, class NT>
template<class VT>	
void Dcsc<IT,NT>::FillColInds(const VT * colnums, IT nind, vector< pair<IT,IT> > & colinds, IT * aux, IT csize) const
{
	if ( aux == NULL || (nzc / nind) < THRESHOLD)   	// use scanning indexing
	{
		IT mink = min(nzc, nind);
		pair<IT,IT> * isect = new pair<IT,IT>[mink];
		pair<IT,IT> * range1 = new pair<IT,IT>[nzc];
		pair<IT,IT> * range2 = new pair<IT,IT>[nind];
		
		for(IT i=0; i < nzc; ++i)
		{
			range1[i] = make_pair(jc[i], i);	// get the actual nonzero value and the index to the ith nonzero
		}
		for(IT i=0; i < nind; ++i)
		{
			range2[i] = make_pair(static_cast<IT>(colnums[i]), 0);	// second is dummy as all the intersecting elements are copied from the first range
		}

		pair<IT,IT> * itr = set_intersection(range1, range1 + nzc, range2, range2+nind, isect, SpHelper::first_compare<IT> );
		// isect now can iterate on a subset of the elements of range1
		// meaning that the intersection can be accessed directly by isect[i] instead of range1[isect[i]]
		// this is because the intersecting elements are COPIED to the output range "isect"

		IT kisect = static_cast<IT>(itr-isect);		// size of the intersection 
		for(IT j=0, i =0; j< nind; ++j)
		{
			// the elements represented by jc[isect[i]] are a subset of the elements represented by colnums[j]
			if( i == kisect || isect[i].first != static_cast<IT>(colnums[j]))
			{
				// not found, signal by setting first = second
				colinds[j].first = 0;
				colinds[j].second = 0;	
			}
			else	// i < kisect && dcsc->jc[isect[i]] == colnums[j]
			{
				IT p = isect[i++].second;
				colinds[j].first = cp[p];
				colinds[j].second = cp[p+1];
			}
		}
		DeleteAll(isect, range1, range2);
	}
	else	 	// use aux based indexing
	{
		bool found;
		for(IT j =0; j< nind; ++j)
		{
			IT pos = AuxIndex(static_cast<IT>(colnums[j]), found, aux, csize);
			if(found)
			{
				colinds[j].first = cp[pos];
				colinds[j].second = cp[pos+1];
			}
			else 	// not found, signal by setting first = second
			{
				colinds[j].first = 0;
				colinds[j].second = 0;
			}
		}
	}
}


template <class IT, class NT>
Dcsc<IT,NT>::~Dcsc()
{
	if(nz > 0)			// dcsc may be empty
	{
		delete[] numx;
		delete[] ir;
	}
	if(nzc > 0)
	{
		delete[] jc;
		delete[] cp;
	}
}

