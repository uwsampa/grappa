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

#ifndef _DCSC_H
#define _DCSC_H

#include <cstdlib>
#include <vector>
#include <limits>
#include <cassert>
#include "SpDefs.h"
#include "SpHelper.h"
#include "StackEntry.h"
#include "MemoryPool.h"
#include "promote.h"

using namespace std;

template <class IT, class NT>
class Dcsc
{
public:
	Dcsc ();
	Dcsc (IT nnz, IT nzcol);

	Dcsc (IT nnz, const vector<IT> & indices, bool isRow); 	//!< Create a logical matrix from (row/column) indices vector
	Dcsc (StackEntry<NT, pair<IT,IT> > * multstack, IT mdim, IT ndim, IT nnz);

	Dcsc (const Dcsc<IT,NT> & rhs);				// copy constructor
	Dcsc<IT,NT> & operator=(const Dcsc<IT,NT> & rhs);	// assignment operator
	Dcsc<IT,NT> & operator+=(const Dcsc<IT,NT> & rhs);	// add and assign operator
	~Dcsc();
	
	bool operator==(const Dcsc<IT,NT> & rhs);	
	template <typename NNT> operator Dcsc<IT,NNT>() const;	//<! numeric type conversion
	template <typename NIT, typename NNT> operator Dcsc<NIT,NNT>() const;	//<! index+numeric type conversion
	
	void EWiseMult(const Dcsc<IT,NT> & rhs, bool exclude); 
	void EWiseScale(NT ** scaler);				//<! scale elements of "this" with the elements dense rhs matrix
	
	template <typename IU, typename NU1, typename NU2>
	friend Dcsc<IU, typename promote_trait<NU1,NU2>::T_promote> EWiseMult(const Dcsc<IU,NU1> & A, const Dcsc<IU,NU2> * B, bool exclude);	// Note that the second parameter is a POINTER

	template <typename _UnaryOperation>
	void Apply(_UnaryOperation __unary_op)
	{	
		transform(numx, numx+nz, numx, __unary_op);
	}

	template <typename _UnaryOperation>
	Dcsc<IT,NT>* Prune(_UnaryOperation __unary_op, bool inPlace);

	IT AuxIndex(const IT colind, bool & found, IT * aux, IT csize) const;
	
	void RowSplit(int numsplits);
	void Split(Dcsc<IT,NT> * & A, Dcsc<IT,NT> * & B, IT cut); 	
	void Merge(const Dcsc<IT,NT> * Adcsc, const Dcsc<IT,NT> * B, IT cut);		

	IT ConstructAux(IT ndim, IT * & aux) const;
	void Resize(IT nzcnew, IT nznew);

	template<class VT>	
	void FillColInds(const VT * colnums, IT nind, vector< pair<IT,IT> > & colinds, IT * aux, IT csize) const;

	Dcsc<IT,NT> & AddAndAssign (StackEntry<NT, pair<IT,IT> > * multstack, IT mdim, IT ndim, IT nnz);

	template <typename _BinaryOperation>
	void UpdateDense(NT ** array, _BinaryOperation __binary_op) const;	// update dense 2D array's entries with __binary_op using elements of "this"

	IT * cp;		//!<  The master array, size nzc+1 (keeps column pointers)
	IT * jc ;		//!<  col indices, size nzc
	IT * ir ;		//!<  row indices, size nz
	NT * numx;		//!<  generic values, size nz
	
	IT nz;
	IT nzc;			//!<  number of columns with at least one non-zero in them

private:
	void getindices (StackEntry<NT, pair<IT,IT> > * multstack, IT & rindex, IT & cindex, IT & j, IT nnz);
};

#include "dcsc.cpp"	
#endif


