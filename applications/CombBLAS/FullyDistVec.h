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

#ifndef _FULLY_DIST_VEC_H_
#define _FULLY_DIST_VEC_H_

#include <iostream>
#include <fstream>
#include <vector>
#include <utility>
#include <iterator>
#include "CombBLAS.h"
#include "CommGrid.h"
#include "FullyDist.h"
#include "Exception.h"

template <class IT, class NT>
class FullyDistSpVec;

template <class IT, class NT, class DER>
class SpParMat;

template <class IT>
class DistEdgeList;

template <class IU, class NU>
class DenseVectorLocalIterator;

// ABAB: As opposed to SpParMat, IT here is used to encode global size and global indices;
// therefore it can not be 32-bits, in general.
template <class IT, class NT>
class FullyDistVec: public FullyDist<IT,NT, typename CombBLAS::disable_if< CombBLAS::is_boolean<NT>::value, NT >::type >
{
public:
	FullyDistVec ( );
	FullyDistVec ( IT globallen, NT initval); 
	FullyDistVec ( shared_ptr<CommGrid> grid);
	FullyDistVec ( shared_ptr<CommGrid> grid, IT globallen, NT initval);
	FullyDistVec ( const FullyDistSpVec<IT, NT> & rhs ); // Sparse -> Dense conversion constructor

	template <class ITRHS, class NTRHS>
	FullyDistVec ( const FullyDistVec<ITRHS, NTRHS>& rhs ); // type converter constructor

	class ScalarReadSaveHandler
	{
	public:
		NT getNoNum(IT index) { return static_cast<NT>(1); }

		template <typename c, typename t>
		NT read(std::basic_istream<c,t>& is, IT index)
		{
			NT v;
			is >> v;
			return v;
		}
	
		template <typename c, typename t>
		void save(std::basic_ostream<c,t>& os, const NT& v, IT index)
		{
			os << v;
		}
	};
	
	template <class HANDLER>
	ifstream& ReadDistribute (ifstream& infile, int master, HANDLER handler);	
	ifstream& ReadDistribute (ifstream& infile, int master) { return ReadDistribute(infile, master, ScalarReadSaveHandler()); }
	
	template <class HANDLER>
	void SaveGathered(ofstream& outfile, int master, HANDLER handler, bool printProcSplits = false);
	void SaveGathered(ofstream& outfile, int master) { SaveGathered(outfile, master, ScalarReadSaveHandler(), false); }


	template <class ITRHS, class NTRHS>
	FullyDistVec<IT,NT> & operator=(const FullyDistVec< ITRHS,NTRHS > & rhs);	// assignment with type conversion
	FullyDistVec<IT,NT> & operator=(const FullyDistVec<IT,NT> & rhs);	//!< Actual assignment operator
	FullyDistVec<IT,NT> & operator=(const FullyDistSpVec<IT,NT> & rhs);		//!< FullyDistSpVec->FullyDistVec conversion operator
	FullyDistVec<IT,NT> & operator=(const DenseParVec<IT,NT> & rhs);		//!< DenseParVec->FullyDistVec conversion operator
	FullyDistVec<IT,NT> operator() (const FullyDistVec<IT,IT> & ri) const;	//<! subsref
	
	//! like operator=, but instead of making a deep copy it just steals the contents. 
	//! Useful for places where the "victim" will be distroyed immediately after the call.
	FullyDistVec<IT,NT> & stealFrom(FullyDistVec<IT,NT> & victim); 
	FullyDistVec<IT,NT> & operator+=(const FullyDistSpVec<IT,NT> & rhs);		
	FullyDistVec<IT,NT> & operator+=(const FullyDistVec<IT,NT> & rhs);
	FullyDistVec<IT,NT> & operator-=(const FullyDistSpVec<IT,NT> & rhs);		
	FullyDistVec<IT,NT> & operator-=(const FullyDistVec<IT,NT> & rhs);
	bool operator==(const FullyDistVec<IT,NT> & rhs) const;

	void SetElement (IT indx, NT numx);	// element-wise assignment
	NT   GetElement (IT indx) const;	// element-wise fetch
	NT operator[](IT indx) const		// more c++ like API
	{
		return GetElement(indx);
	}

	void Set(const FullyDistSpVec< IT,NT > & rhs);
	void iota(IT globalsize, NT first);
	void RandPerm();	// randomly permute the vector
	FullyDistVec<IT,IT> sort();	// sort and return the permutation

	using FullyDist<IT,NT,typename CombBLAS::disable_if< CombBLAS::is_boolean<NT>::value, NT >::type>::LengthUntil;
	using FullyDist<IT,NT,typename CombBLAS::disable_if< CombBLAS::is_boolean<NT>::value, NT >::type>::TotalLength;
	using FullyDist<IT,NT,typename CombBLAS::disable_if< CombBLAS::is_boolean<NT>::value, NT >::type>::Owner;
	using FullyDist<IT,NT,typename CombBLAS::disable_if< CombBLAS::is_boolean<NT>::value, NT >::type>::MyLocLength;
	IT LocArrSize() const { return arr.size(); }	// = MyLocLength() once arr is resized
	
	template <typename _Predicate>
	FullyDistSpVec<IT,NT> Find(_Predicate pred) const;	//!< Return the elements for which pred is true

	template <typename _Predicate>
	FullyDistVec<IT,IT> FindInds(_Predicate pred) const;	//!< Return the indices where pred is true

	template <typename _Predicate>
	IT Count(_Predicate pred) const;	//!< Return the number of elements for which pred is true

	template <typename _UnaryOperation>
	void Apply(_UnaryOperation __unary_op)
	{	
		transform(arr.begin(), arr.end(), arr.begin(), __unary_op);
	}
	
	template <typename _BinaryOperation>
	void ApplyInd(_BinaryOperation __binary_op)
	{
		IT offset = LengthUntil();
		#ifdef _OPENMP
		#pragma omp parallel for
		#endif
		for(IT i=0; (unsigned)i < arr.size(); ++i)
			arr[i] = __binary_op(arr[i], i + offset);
	}

	template <typename _UnaryOperation, typename IRRELEVANT_NT>
	void Apply(_UnaryOperation __unary_op, const FullyDistSpVec<IT,IRRELEVANT_NT>& mask);

	// extended callback versions
	template <typename _BinaryOperation, typename _BinaryPredicate, class NT2>
	void EWiseApply(const FullyDistVec<IT,NT2> & other, _BinaryOperation __binary_op, _BinaryPredicate _do_op, const bool useExtendedBinOp);
	template <typename _BinaryOperation, typename _BinaryPredicate, class NT2>
	void EWiseApply(const FullyDistSpVec<IT,NT2> & other, _BinaryOperation __binary_op, _BinaryPredicate _do_op, bool applyNulls, NT2 nullValue, const bool useExtendedBinOp);

	// plain fallback versions
	template <typename _BinaryOperation, typename _BinaryPredicate, class NT2>
	void EWiseApply(const FullyDistVec<IT,NT2> & other, _BinaryOperation __binary_op, _BinaryPredicate _do_op)
	{
		EWiseApply(other,
					EWiseExtToPlainAdapter<NT, NT, NT2, _BinaryOperation>(__binary_op),
					EWiseExtToPlainAdapter<bool, NT, NT2, _BinaryPredicate>(_do_op),
					true);
	}
	template <typename _BinaryOperation, typename _BinaryPredicate, class NT2>
	void EWiseApply(const FullyDistSpVec<IT,NT2> & other, _BinaryOperation __binary_op, _BinaryPredicate _do_op, bool applyNulls, NT2 nullValue)
	{
		EWiseApply(other,
					EWiseExtToPlainAdapter<NT, NT, NT2, _BinaryOperation>(__binary_op),
					EWiseExtToPlainAdapter<bool, NT, NT2, _BinaryPredicate>(_do_op),
					applyNulls, nullValue, true);
	}


	template <typename T1, typename T2>
	class retTrue {
		public:
		bool operator()(const T1& x, const T2& y)
		{
			return true;
		}
	};

	template <typename _BinaryOperation, class NT2>
	void EWiseApply(const FullyDistVec<IT,NT2> & other, _BinaryOperation __binary_op)
	{
		this->EWiseApply(other, __binary_op, retTrue<NT, NT2>());
	}
	template <typename _BinaryOperation, class NT2>
	void EWiseApply(const FullyDistSpVec<IT,NT2> & other, _BinaryOperation __binary_op, bool applyNulls, NT2 nullValue)
	{
		this->EWiseApply(other, __binary_op, retTrue<NT, NT2>(), applyNulls, nullValue);
	}
	
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
	shared_ptr<CommGrid> getcommgrid() const { return commGrid; }
	
	template <typename _BinaryOperation>
	NT Reduce(_BinaryOperation __binary_op, NT identity);	//! Reduce can be used to implement max_element, for instance

	template <typename OUT, typename _BinaryOperation, typename _UnaryOperation>
	OUT Reduce(_BinaryOperation __binary_op, OUT default_val, _UnaryOperation __unary_op);

	void SelectCandidates(double nver, bool deterministic);

	using FullyDist<IT,NT,typename CombBLAS::disable_if< CombBLAS::is_boolean<NT>::value, NT >::type>::glen; 
	using FullyDist<IT,NT,typename CombBLAS::disable_if< CombBLAS::is_boolean<NT>::value, NT >::type>::commGrid; 

private:
	vector< NT > arr;

	template <typename _BinaryOperation>	
	void EWise(const FullyDistVec<IT,NT> & rhs,  _BinaryOperation __binary_op);

	template <class IU, class NU>
	friend class DenseParMat;

	template <class IU, class NU, class UDER>
	friend class SpParMat;

	template <class IU, class NU>
	friend class FullyDistVec;

	template <class IU, class NU>
	friend class FullyDistSpVec;
	
	template <class IU, class NU>
	friend class DenseVectorLocalIterator;

	template <typename SR, typename IU, typename NUM, typename NUV, typename UDER> 
	friend FullyDistVec<IU,typename promote_trait<NUM,NUV>::T_promote> 
	SpMV (const SpParMat<IU,NUM,UDER> & A, const FullyDistVec<IU,NUV> & x );

	template <typename IU, typename NU1, typename NU2>
	friend FullyDistSpVec<IU,typename promote_trait<NU1,NU2>::T_promote> 
	EWiseMult (const FullyDistSpVec<IU,NU1> & V, const FullyDistVec<IU,NU2> & W , bool exclude, NU2 zero);

	template <typename IU, typename NU1, typename NU2, typename _BinaryOperation>
	friend FullyDistSpVec<IU,typename promote_trait<NU1,NU2>::T_promote> 
	EWiseApply (const FullyDistSpVec<IU,NU1> & V, const FullyDistVec<IU,NU2> & W , _BinaryOperation _binary_op, typename promote_trait<NU1,NU2>::T_promote zero);

	template <typename RET, typename IU, typename NU1, typename NU2, typename _BinaryOperation, typename _BinaryPredicate>
	friend FullyDistSpVec<IU,RET> 
	EWiseApply (const FullyDistSpVec<IU,NU1> & V, const FullyDistVec<IU,NU2> & W , _BinaryOperation _binary_op, _BinaryPredicate _doOp, bool allowVNulls, NU1 Vzero, const bool useExtendedBinOp);

	template <typename IU>
	friend void RenameVertices(DistEdgeList<IU> & DEL);
	
	template <typename IU, typename NU>
	friend FullyDistVec<IU,NU> Concatenate ( vector< FullyDistVec<IU,NU> > & vecs);
};

#include "FullyDistVec.cpp"
#endif


