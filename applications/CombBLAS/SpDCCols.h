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

#ifndef _SP_DCCOLS_H
#define _SP_DCCOLS_H

#include <iostream>
#include <fstream>
#include <cmath>
#include "SpMat.h"	// Best to include the base class first
#include "SpHelper.h"
#include "StackEntry.h"
#include "dcsc.h"
#include "Isect.h"
#include "Semirings.h"
#include "MemoryPool.h"
#include "LocArr.h"
#include "Friends.h"
#include "CombBLAS.h"
#include "FullyDist.h"


template <class IT, class NT>
class SpDCCols: public SpMat<IT, NT, SpDCCols<IT, NT> >
{
public:
	typedef IT LocalIT;
	typedef NT LocalNT;

	// Constructors :
	SpDCCols ();
	SpDCCols (IT size, IT nRow, IT nCol, IT nzc);
	SpDCCols (const SpTuples<IT,NT> & rhs, bool transpose);
	SpDCCols (const SpDCCols<IT,NT> & rhs);					// Actual copy constructor		
	~SpDCCols();

	template <typename NNT> operator SpDCCols<IT,NNT> () const;		//!< NNT: New numeric type
	template <typename NIT, typename NNT> operator SpDCCols<NIT,NNT> () const;		//!< NNT: New numeric type, NIT: New index type

	// Member Functions and Operators: 
	SpDCCols<IT,NT> & operator= (const SpDCCols<IT, NT> & rhs);
	SpDCCols<IT,NT> & operator+= (const SpDCCols<IT, NT> & rhs);
	SpDCCols<IT,NT> operator() (IT ri, IT ci) const;	
	SpDCCols<IT,NT> operator() (const vector<IT> & ri, const vector<IT> & ci) const;
	bool operator== (const SpDCCols<IT, NT> & rhs) const
	{
		if(rhs.nnz == 0 && nnz == 0)
			return true;
		if(nnz != rhs.nnz || m != rhs.m || n != rhs.n)
			return false;
		return ((*dcsc) == (*(rhs.dcsc)));
	}

	class SpColIter //! Iterate over (sparse) columns of the sparse matrix
	{
	public:
		class NzIter	//! Iterate over the nonzeros of the sparse column
		{	
		public:
			NzIter(IT * ir, NT * num) : rid(ir), val(num) {}
     		
      			bool operator==(const NzIter & other)
      			{
       		  		return(rid == other.rid);	// compare pointers
      			}
      			bool operator!=(const NzIter & other)
      			{
        	 		return(rid != other.rid);
      			}
           		NzIter & operator++()	// prefix operator
      			{
         			++rid;
				++val;
         			return(*this);
			}
      			NzIter operator++(int)	// postfix operator
      			{
         			NzIter tmp(*this);
         			++(*this);
        	 		return(tmp);
      			}
			IT rowid() const	//!< Return the "local" rowid of the current nonzero entry.
			{
				return (*rid);
			}
			NT & value()		//!< value is returned by reference for possible updates
			{
				return (*val);
			}
		private:
			IT * rid;
			NT * val;
			
		};

      		SpColIter(IT * cp, IT * jc) : cptr(cp), cid(jc) {}
     		
      		bool operator==(const SpColIter& other)
      		{
         		return(cptr == other.cptr);	// compare pointers
      		}
      		bool operator!=(const SpColIter& other)
      		{
         		return(cptr != other.cptr);
      		}
           	SpColIter& operator++()		// prefix operator
      		{
         		++cptr;
			++cid;
         		return(*this);
      		}
      		SpColIter operator++(int)	// postfix operator
      		{
         		SpColIter tmp(*this);
         		++(*this);
         		return(tmp);
      		}
      		IT colid() const	//!< Return the "local" colid of the current column. 
      		{
         		return (*cid);
      		}
		IT colptr() const
		{
			return (*cptr);
		}
		IT colptrnext() const
		{
			return (*(cptr+1));
		}
		IT nnz() const
		{
			return (colptrnext() - colptr());
		}
  	private:
      		IT * cptr;
		IT * cid;
   	};
	
	SpColIter begcol()
	{
		if( nnz > 0 )
			return SpColIter(dcsc->cp, dcsc->jc); 
		else	
			return SpColIter(NULL, NULL);
	}	
	SpColIter endcol()
	{
		if( nnz > 0 )
			return SpColIter(dcsc->cp + dcsc->nzc, NULL); 
		else
			return SpColIter(NULL, NULL);
	}

	typename SpColIter::NzIter begnz(const SpColIter & ccol)	//!< Return the beginning iterator for the nonzeros of the current column
	{
		return typename SpColIter::NzIter( dcsc->ir + ccol.colptr(), dcsc->numx + ccol.colptr() );
	}

	typename SpColIter::NzIter endnz(const SpColIter & ccol)	//!< Return the ending iterator for the nonzeros of the current column
	{
		return typename SpColIter::NzIter( dcsc->ir + ccol.colptrnext(), NULL );
	}			

	template <typename _UnaryOperation>
	void Apply(_UnaryOperation __unary_op)
	{
		if(nnz > 0)
			dcsc->Apply(__unary_op);	
	}
	
	template <typename _UnaryOperation>
	SpDCCols<IT,NT>* Prune(_UnaryOperation __unary_op, bool inPlace);

	template <typename _BinaryOperation>
	void UpdateDense(NT ** array, _BinaryOperation __binary_op) const
	{
		if(nnz > 0 && dcsc != NULL)
			dcsc->UpdateDense(array, __binary_op);
	}

	void EWiseScale(NT ** scaler, IT m_scaler, IT n_scaler);
	void EWiseMult (const SpDCCols<IT,NT> & rhs, bool exclude);
	
	void Transpose();				//!< Mutator version, replaces the calling object 
	SpDCCols<IT,NT> TransposeConst() const;		//!< Const version, doesn't touch the existing object

	void RowSplit(int numsplits)
	{
		BooleanRowSplit(*this, numsplits);	// only works with boolean arrays
	}

	void Split(SpDCCols<IT,NT> & partA, SpDCCols<IT,NT> & partB); 	//!< \attention Destroys calling object (*this)
	void Merge(SpDCCols<IT,NT> & partA, SpDCCols<IT,NT> & partB);	//!< \attention Destroys its parameters (partA & partB)

	void CreateImpl(const vector<IT> & essentials);
	void CreateImpl(IT size, IT nRow, IT nCol, tuple<IT, IT, NT> * mytuples);

	Arr<IT,NT> GetArrays() const;
	vector<IT> GetEssentials() const;
	const static IT esscount;

	bool isZero() const { return (nnz == 0); }
	IT getnrow() const { return m; }
	IT getncol() const { return n; }
	IT getnnz() const { return nnz; }
	IT getnzc() const { return (nnz == 0) ? 0: dcsc->nzc; }
	int getnsplit() const { return splits; }
	
	ofstream& put(ofstream & outfile) const;
	ifstream& get(ifstream & infile);
	void PrintInfo() const;
	void PrintInfo(ofstream & out) const;

	template <typename SR> 
	int PlusEq_AtXBt(const SpDCCols<IT,NT> & A, const SpDCCols<IT,NT> & B);  
	
	template <typename SR>
	int PlusEq_AtXBn(const SpDCCols<IT,NT> & A, const SpDCCols<IT,NT> & B);
	
	template <typename SR>
	int PlusEq_AnXBt(const SpDCCols<IT,NT> & A, const SpDCCols<IT,NT> & B);  
	
	template <typename SR>
	int PlusEq_AnXBn(const SpDCCols<IT,NT> & A, const SpDCCols<IT,NT> & B);
	
	Dcsc<IT, NT> * GetDCSC() const 	// only for single threaded matrices
	{
		return dcsc;
	}

	Dcsc<IT, NT> * GetDCSC(int i) const 	// only for split (multithreaded) matrices
	{
		return dcscarr[i];
	}

private:
	void CopyDcsc(Dcsc<IT,NT> * source);
	SpDCCols<IT,NT> ColIndex(const vector<IT> & ci) const;	//!< col indexing without multiplication	

	template <typename SR, typename NTR>
	SpDCCols< IT, typename promote_trait<NT,NTR>::T_promote > OrdOutProdMult(const SpDCCols<IT,NTR> & rhs) const;	

	template <typename SR, typename NTR>
	SpDCCols< IT, typename promote_trait<NT,NTR>::T_promote > OrdColByCol(const SpDCCols<IT,NTR> & rhs) const;	
	
	SpDCCols (IT size, IT nRow, IT nCol, const vector<IT> & indices, bool isRow);	// Constructor for indexing
	SpDCCols (IT nRow, IT nCol, Dcsc<IT,NT> * mydcsc);			// Constructor for multiplication

	// Anonymous union
	union {
		Dcsc<IT, NT> * dcsc;
		Dcsc<IT, NT> ** dcscarr;
	};

	IT m;
	IT n;
	IT nnz;
	
	int splits;	// ABAB: Future multithreaded extension

	template <class IU, class NU>
	friend class SpDCCols;		// Let other template instantiations (of the same class) access private members
	
	template <class IU, class NU>
	friend class SpTuples;

	template <class IU, class NU>
	friend class SpDCCols<IU, NU>::SpColIter;
	
	template<typename IU>
	friend void BooleanRowSplit(SpDCCols<IU, bool> & A, int numsplits);

	template<typename IU, typename NU1, typename NU2>
	friend SpDCCols<IU, typename promote_trait<NU1,NU2>::T_promote > EWiseMult (const SpDCCols<IU,NU1> & A, const SpDCCols<IU,NU2> & B, bool exclude);

	template<typename N_promote, typename IU, typename NU1, typename NU2, typename _BinaryOperation>
	friend SpDCCols<IU, N_promote > EWiseApply (const SpDCCols<IU,NU1> & A, const SpDCCols<IU,NU2> & B, _BinaryOperation __binary_op, bool notB, const NU2& defaultBVal);

	template <typename RETT, typename IU, typename NU1, typename NU2, typename _BinaryOperation, typename _BinaryPredicate> 
	friend SpDCCols<IU,RETT> EWiseApply (const SpDCCols<IU,NU1> & A, const SpDCCols<IU,NU2> & B, _BinaryOperation __binary_op, _BinaryPredicate do_op, bool allowANulls, bool allowBNulls, const NU1& ANullVal, const NU2& BNullVal, const bool allowIntersect);

	template<class SR, class NUO, class IU, class NU1, class NU2>
	friend SpTuples<IU, NUO> * Tuples_AnXBn 
		(const SpDCCols<IU, NU1> & A, const SpDCCols<IU, NU2> & B, bool clearA, bool clearB);

	template<class SR, class NUO, class IU, class NU1, class NU2>
	friend SpTuples<IU, NUO> * Tuples_AnXBt 
		(const SpDCCols<IU, NU1> & A, const SpDCCols<IU, NU2> & B, bool clearA, bool clearB);

	template<class SR, class NUO, class IU, class NU1, class NU2>
	friend SpTuples<IU, NUO> * Tuples_AtXBn 
		(const SpDCCols<IU, NU1> & A, const SpDCCols<IU, NU2> & B, bool clearA, bool clearB);

	template<class SR, class NUO, class IU, class NU1, class NU2>
	friend SpTuples<IU, NUO> * Tuples_AtXBt 
		(const SpDCCols<IU, NU1> & A, const SpDCCols<IU, NU2> & B, bool clearA, bool clearB);

	template <typename SR, typename IU, typename NU, typename RHS, typename LHS>
	friend void dcsc_gespmv (const SpDCCols<IU, NU> & A, const RHS * x, LHS * y);

	template <typename SR, typename IU, typename NUM, typename IVT, typename OVT>	
	friend int dcsc_gespmv_threaded (const SpDCCols<IU, NUM> & A, const int32_t * indx, const IVT * numx, int32_t nnzx, 
		int32_t * & sendindbuf, OVT * & sendnumbuf, int * & sdispls, int p_c);
	
	template <typename SR, typename IU, typename NUM, typename IVT, typename OVT>
	friend void dcsc_gespmv_threaded_setbuffers (const SpDCCols<IU, NUM> & A, const int32_t * indx, const IVT * numx, int32_t nnzx, 
		int32_t * sendindbuf, OVT * sendnumbuf, int * cnts, int * sdispls, int p_c);
};

// At this point, complete type of of SpDCCols is known, safe to declare these specialization (but macros won't work as they are preprocessed)
// General case #1: When both NT is the same
template <class IT, class NT> struct promote_trait< SpDCCols<IT,NT> , SpDCCols<IT,NT> >          
	{                                           
        typedef SpDCCols<IT,NT> T_promote;                    
    };
// General case #2: First is boolean the second is anything except boolean (to prevent ambiguity) 
template <class IT, class NT> struct promote_trait< SpDCCols<IT,bool> , SpDCCols<IT,NT>, typename CombBLAS::disable_if< CombBLAS::is_boolean<NT>::value >::type >      
    {                                           
        typedef SpDCCols<IT,NT> T_promote;                    
    };
// General case #3: Second is boolean the first is anything except boolean (to prevent ambiguity) 
template <class IT, class NT> struct promote_trait< SpDCCols<IT,NT> , SpDCCols<IT,bool>, typename CombBLAS::disable_if< CombBLAS::is_boolean<NT>::value >::type >      
	{                                           
		typedef SpDCCols<IT,NT> T_promote;                    
	};
template <class IT> struct promote_trait< SpDCCols<IT,int> , SpDCCols<IT,float> >       
    {                                           
        typedef SpDCCols<IT,float> T_promote;                    
    };

template <class IT> struct promote_trait< SpDCCols<IT,float> , SpDCCols<IT,int> >       
    {                                           
        typedef SpDCCols<IT,float> T_promote;                    
    };
template <class IT> struct promote_trait< SpDCCols<IT,int> , SpDCCols<IT,double> >       
    {                                           
        typedef SpDCCols<IT,double> T_promote;                    
    };
template <class IT> struct promote_trait< SpDCCols<IT,double> , SpDCCols<IT,int> >       
    {                                           
        typedef SpDCCols<IT,double> T_promote;                    
    };


// Below are necessary constructs to be able to define a SpMat<NT,IT> where
// all we know is DER (say SpDCCols<int, double>) and NT,IT
// in other words, we infer the templated SpDCCols<> type
// This is not a type conversion from an existing object, 
// but a type inference for the newly created object
// NIT: New IT, NNT: New NT
template <class DER, class NIT, class NNT>
struct create_trait
{
	// none
};

// Capture everything of the form SpDCCols<OIT, ONT>
// it may come as a surprise that the partial specializations can 
// involve more template parameters than the primary template
template <class NIT, class NNT, class OIT, class ONT>
struct create_trait< SpDCCols<OIT, ONT> , NIT, NNT >
{
     typedef SpDCCols<NIT,NNT> T_inferred;
};

#include "SpDCCols.cpp"
#endif

