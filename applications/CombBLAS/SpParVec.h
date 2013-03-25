#ifndef _SP_PAR_VEC_H_
#define _SP_PAR_VEC_H_

#include <iostream>
#include <vector>
#include <utility>
#include "CombBLAS.h"
#include "CommGrid.h"
#include "promote.h"
#include "SpParMat.h"
#include "Operations.h"

template <class IT, class NT, class DER>
class SpParMat;

template <class IT>
class DistEdgeList;


/** 
  * A sparse vector of length n (with nnz <= n of them being nonzeros) is distributed to 
  * diagonal processors in a way that respects ordering of the nonzero indices
  * Example: x = [5,1,6,2,9] for nnz(x)=5 and length(x)=10 
  *	we use 4 processors P_00, P_01, P_10, P_11
  * 	Then P_00 owns [1,2] (in the range [0...4]) and P_11 owns rest
  * In the case of A(v,w) type sparse matrix indexing, this doesn't matter because n = nnz
  * 	After all, A(v,w) will have dimensions length(v) x length (w) 
  * 	v and w will be of numerical type (NT) "int" and their indices (IT) will be consecutive integers 
  * It is possibly that nonzero counts are distributed unevenly
  * Example: x=[1,2,3,4,5] and length(x) = 10, then P_00 would own all the nonzeros and P_11 would hold an empty vector
  * Just like in SpParMat case, indices are local to processors (they belong to range [0,...,length-1] on each processor)
  *
  * TODO: Instead of repeated calls to "DiagWorld", this class should be oblivious to the communicator
  * 	  It should just distribute the vector to the MPI::IntraComm that it owns, whether diagonal or whole
 **/
  
template <class IT, class NT>
class SpParVec
{
public:
	SpParVec ( );
	SpParVec ( IT loclength );
	SpParVec ( shared_ptr<CommGrid> grid);
	SpParVec ( shared_ptr<CommGrid> grid, IT loclength);

	//! like operator=, but instead of making a deep copy it just steals the contents. 
	//! Useful for places where the "victim" will be distroyed immediately after the call.
	void stealFrom(SpParVec<IT,NT> & victim); 
	SpParVec<IT,NT> & operator+=(const SpParVec<IT,NT> & rhs);
	SpParVec<IT,NT> & operator-=(const SpParVec<IT,NT> & rhs);
	ifstream& ReadDistribute (ifstream& infile, int master);	

	template <typename NNT> operator SpParVec< IT,NNT > () const	//!< Type conversion operator
	{
		SpParVec<IT,NNT> CVT(commGrid);
		CVT.ind = vector<IT>(ind.begin(), ind.end());
		CVT.num = vector<NNT>(num.begin(), num.end());
		CVT.length = length;
	}

	void PrintInfo(string vecname) const;
	void iota(IT size, NT first);
	SpParVec<IT,NT> operator() (const SpParVec<IT,IT> & ri) const;	//!< SpRef (expects NT of ri to be 0-based)
	void SetElement (IT indx, NT numx);	// element-wise assignment
	NT operator[](IT indx) const;

	// sort the vector itself
	// return the permutation vector (0-based)
	SpParVec<IT, IT> sort();	

	IT getlocnnz() const 
	{
		return ind.size();
	}
	
	IT getnnz() const
	{
		IT totnnz = 0;
		IT locnnz = ind.size();
		MPI_Allreduce( &locnnz, & totnnz, 1, MPIType<IT>(), MPI_SUM, commGrid->GetWorld());
		return totnnz;
	}

	IT getTypicalLocLength() const;
	IT getTotalLength(MPI_Comm & comm) const;
	IT getTotalLength() const { return getTotalLength(commGrid->GetWorld()); }
	
	void setNumToInd()
	{
		MPI_Comm DiagWorld = commGrid->GetDiagWorld();
        	if(DiagWorld != MPI_COMM_NULL) // Diagonal processors only
        	{
			int rank;
			MPI_Comm_rank(DiagWorld,&rank);

            		IT n_perproc = getTypicalLocLength();
            		IT offset = static_cast<IT>(rank) * n_perproc;

            		transform(ind.begin(), ind.end(), num.begin(), bind2nd(plus<IT>(), offset));
		}
	}

	template <typename _UnaryOperation>
	void Apply(_UnaryOperation __unary_op)
	{
		transform(num.begin(), num.end(), num.begin(), __unary_op);
	}

	template <typename _BinaryOperation>
	NT Reduce(_BinaryOperation __binary_op, NT init);

	void DebugPrint();
	shared_ptr<CommGrid> getcommgrid() { return commGrid; }
	NT NOT_FOUND; 
private:
	shared_ptr<CommGrid> commGrid;
	vector< IT > ind;	// ind.size() give the number of nonzeros
	vector< NT > num;
	IT length;		// actual local length of the vector (including zeros)
	bool diagonal;

	template <class IU, class NU>
	friend class SpParVec;

	template <class IU, class NU>
	friend class DenseParVec;
	
	template <class IU, class NU, class UDER>
	friend class SpParMat;

	template <typename SR, typename IU, typename NUM, typename NUV, typename UDER> 
	friend SpParVec<IU,typename promote_trait<NUM,NUV>::T_promote> 
	SpMV (const SpParMat<IU,NUM,UDER> & A, const SpParVec<IU,NUV> & x );

	template <typename IU, typename NU1, typename NU2>
	friend SpParVec<IU,typename promote_trait<NU1,NU2>::T_promote> 
	EWiseMult (const SpParVec<IU,NU1> & V, const DenseParVec<IU,NU2> & W , bool exclude, NU2 zero);

	template <typename IU>
	friend void RandPerm(SpParVec<IU,IU> & V); 	// called on an existing object, randomly permutes it
	
	template <typename IU>
	friend void RenameVertices(DistEdgeList<IU> & DEL);
};

#include "SpParVec.cpp"
#endif

