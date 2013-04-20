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

#ifndef _DENSE_PAR_VEC_H_
#define _DENSE_PAR_VEC_H_

#include <iostream>
#include <fstream>
#include <vector>
#include <utility>
#include <iterator>
#include "CombBLAS.h"
#include "CommGrid.h"

template <class IT, class NT>
class SpParVec;

template <class IT, class NT, class DER>
class SpParMat;

template <class IT>
class DistEdgeList;

template <class IT, class NT>
class DenseParVec
{
public:
	DenseParVec ( );
	DenseParVec ( IT globallength );
	DenseParVec ( IT locallength, NT initval, NT id); // initializes the vector to size locallength (if this node is on a diagonal)
	DenseParVec ( shared_ptr<CommGrid> grid, NT id);
	DenseParVec ( shared_ptr<CommGrid> grid, IT locallength, NT initval, NT id);
	
	ifstream& ReadDistribute (ifstream& infile, int master);
	DenseParVec<IT,NT> & operator=(const SpParVec<IT,NT> & rhs);		//!< SpParVec->DenseParVec conversion operator
	DenseParVec<IT,NT> & operator=(const DenseParVec<IT,NT> & rhs);	
	DenseParVec<IT,NT> operator() (const DenseParVec<IT,IT> & ri) const;	//<! subsref
	
	//! like operator=, but instead of making a deep copy it just steals the contents. 
	//! Useful for places where the "victim" will be distroyed immediately after the call.
	DenseParVec<IT,NT> & stealFrom(DenseParVec<IT,NT> & victim); 
	DenseParVec<IT,NT> & operator+=(const SpParVec<IT,NT> & rhs);		
	DenseParVec<IT,NT> & operator+=(const DenseParVec<IT,NT> & rhs);
	DenseParVec<IT,NT> & operator-=(const SpParVec<IT,NT> & rhs);		
	DenseParVec<IT,NT> & operator-=(const DenseParVec<IT,NT> & rhs);
	bool operator==(const DenseParVec<IT,NT> & rhs) const;

	void SetElement (IT indx, NT numx);	// element-wise assignment
	NT   GetElement (IT indx) const;	// element-wise fetch
	NT operator[](IT indx) const		// more c++ like API
	{
		return GetElement(indx);
	} 
	
	void iota(IT size, NT first);
	void RandPerm();	// randomly permute the vector

	IT getTypicalLocLength() const;
	IT getTotalLength(MPI_Comm & comm) const;
	IT getTotalLength() const { return getTotalLength(commGrid->GetWorld()); }
	IT getLocalLength() const { return arr.size(); }
	
	template <typename _Predicate>
	SpParVec<IT,NT> Find(_Predicate pred) const;	//!< Return the elements for which pred is true

	template <typename _Predicate>
	DenseParVec<IT,IT> FindInds(_Predicate pred) const;	//!< Return the indices where pred is true

	template <typename _Predicate>
	IT Count(_Predicate pred) const;	//!< Return the number of elements for which pred is true

	template <typename _UnaryOperation>
	void Apply(_UnaryOperation __unary_op)
	{	
		transform(arr.begin(), arr.end(), arr.begin(), __unary_op);
	}	

	template <typename _UnaryOperation>
	void Apply(_UnaryOperation __unary_op, const SpParVec<IT,NT>& mask);
	
	void PrintToFile(string prefix)
	{
		ofstream output;
		commGrid->OpenDebugFile(prefix, output);
		copy(arr.begin(), arr.end(), ostream_iterator<NT> (output, " "));
		output << endl;
		output.close();
	}

	void PrintInfo(string vectorname) const;
	void DebugPrint();
	shared_ptr<CommGrid> getcommgrid() { return commGrid; }
	
	template <typename _BinaryOperation>
	NT Reduce(_BinaryOperation __binary_op, NT identity);	//! Reduce can be used to implement max_element, for instance
			
private:
	shared_ptr<CommGrid> commGrid;
	vector< NT > arr;
	bool diagonal;
	NT zero;	//!< the element for non-existings scalars (0.0 for a vector on Reals, +infinity for a vector on the tropical semiring) 

	template <typename _BinaryOperation>	
	void EWise(const DenseParVec<IT,NT> & rhs,  _BinaryOperation __binary_op);

	template <class IU, class NU>
	friend class DenseParMat;

	template <class IU, class NU, class UDER>
	friend class SpParMat;

	template <class IU, class NU>
	friend class DenseParVec;

	template <class IU, class NU>
	friend class FullyDistVec;

	template <typename SR, typename IU, typename NUM, typename NUV, typename UDER> 
	friend DenseParVec<IU,typename promote_trait<NUM,NUV>::T_promote> 
	SpMV (const SpParMat<IU,NUM,UDER> & A, const DenseParVec<IU,NUV> & x );

	template <typename IU, typename NU1, typename NU2>
	friend SpParVec<IU,typename promote_trait<NU1,NU2>::T_promote> 
	EWiseMult (const SpParVec<IU,NU1> & V, const DenseParVec<IU,NU2> & W , bool exclude, NU2 zero);

	template <typename IU>
	friend void RenameVertices(DistEdgeList<IU> & DEL);
};

#include "DenseParVec.cpp"
#endif


