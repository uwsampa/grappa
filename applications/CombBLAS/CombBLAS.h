#ifndef COMBBLAS_H
#define COMBBLAS_H

// These macros should be defined before stdint.h is included
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#include <stdint.h>

#if defined(COMBBLAS_BOOST)
	#ifdef CRAYCOMP
		#include <boost/config/compiler/cray.hpp>
	#endif
	#include <boost/tr1/memory.hpp>
	#include <boost/tr1/unordered_map.hpp>
	#include <boost/tr1/tuple.hpp>
 	using namespace boost;
	#define joker boost	// namespace
#elif defined(COMBBLAS_TR1)
	#include <tr1/memory>
	#include <tr1/unordered_map>
	#include <tr1/tuple>
 	#include <tr1/type_traits>
	using namespace std::tr1;
	#define joker std::tr1
#elif defined(_MSC_VER) && (_MSC_VER < 1600)
	#include <memory>
	#include <unordered_map>
	#include <tuple>
	#include <type_traits>
	using namespace std::tr1;
	#define joker std::tr1
#else // C++11
	#include <memory>
	#include <unordered_map>
	#include <tuple>
	#include <type_traits>
	#define joker std
#endif
using namespace std;
// for VC2008

//#ifdef _MSC_VER
//#pragma warning( disable : 4244 ) // conversion from 'int64_t' to 'double', possible loss of data
//#endif

extern double cblas_alltoalltime;
extern double cblas_allgathertime;
extern double cblas_localspmvtime;
extern double cblas_mergeconttime;
extern double cblas_transvectime;

// An adapter function that allows using extended-callback EWiseApply with plain-old binary functions that don't want the extra parameters.
template <typename RETT, typename NU1, typename NU2, typename BINOP>
class EWiseExtToPlainAdapter
{
	public:
	BINOP plain_binary_op;
	
	EWiseExtToPlainAdapter(BINOP op): plain_binary_op(op) {}
	
	RETT operator()(const NU1& a, const NU2& b, bool aIsNull, bool bIsNull)
	{
		return plain_binary_op(a, b);
	}
};

#include "SpDefs.h"
#include "BitMap.h"
#include "SpTuples.h"
#include "SpDCCols.h"
#include "SpParMat.h"
#include "FullyDistVec.h"
#include "FullyDistSpVec.h"
#include "VecIterator.h"
#include "ParFriends.h"
#include "BFSFriends.h"
#include "DistEdgeList.h"
#include "Semirings.h"
#include "Operations.h"
#include "MPIType.h"

#endif
