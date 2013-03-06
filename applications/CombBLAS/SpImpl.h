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

#ifndef _SP_IMPL_H_
#define _SP_IMPL_H_

#include <iostream>
#include <vector>
using namespace std;

template <class IT, class NT>
class Dcsc;

template <class SR, class IT, class NUM, class IVT, class OVT>
struct SpImpl;

template <class SR, class IT, class NUM, class IVT, class OVT>
void SpMXSpV(const Dcsc<IT,NUM> & Adcsc, int32_t mA, const int32_t * indx, const IVT * numx, int32_t veclen,  
			 vector<int32_t> & indy, vector< OVT > & numy)
{
	SpImpl<SR,IT,NUM,IVT,OVT>::SpMXSpV(Adcsc, mA, indx, numx, veclen, indy, numy);	// don't touch this
};

template <class SR, class IT, class NUM, class IVT, class OVT>
void SpMXSpV(const Dcsc<IT,NUM> & Adcsc, int32_t mA, const int32_t * indx, const IVT * numx, int32_t veclen,  
			 int32_t * indy, OVT * numy, int * cnts, int * dspls, int p_c)
{
	SpImpl<SR,IT,NUM,IVT,OVT>::SpMXSpV(Adcsc, mA, indx, numx, veclen, indy, numy, cnts, dspls,p_c);	// don't touch this
};

template <class SR, class IT, class NUM, class IVT, class OVT>
void SpMXSpV_ForThreading(const Dcsc<IT,NUM> & Adcsc, int32_t mA, const int32_t * indx, const IVT * numx, int32_t veclen,  
		vector<int32_t> & indy, vector< OVT > & numy, int32_t offset)
{
	SpImpl<SR,IT,NUM,IVT,OVT>::SpMXSpV_ForThreading(Adcsc, mA, indx, numx, veclen, indy, numy, offset);	// don't touch this
};

/**
 * IT: The sparse matrix index type. Sparse vector index type is fixed to be int32_t
 * It is the caller function's (inside ParFriends/Friends) job to convert any different types
 * and ensure correctness. Rationale is efficiency, and the fact that we know for sure 
 * that 32-bit LOCAL indices are sufficient for all reasonable concurrencies and data sizes (as of 2011)
 **/
template <class SR, class IT, class NUM, class IVT, class OVT>
struct SpImpl
{
	static void SpMXSpV(const Dcsc<IT,NUM> & Adcsc, int32_t mA, const int32_t * indx, const IVT * numx, int32_t veclen,  
			vector<int32_t> & indy, vector< OVT > & numy);	// specialize this

	static void SpMXSpV(const Dcsc<IT,NUM> & Adcsc, int32_t mA, const int32_t * indx, const IVT * numx, int32_t veclen,  
			int32_t * indy, OVT * numy, int * cnts, int * dspls, int p_c)
	{
		cout << "Optbuf enabled version is not yet supported with general (non-boolean) matrices" << endl;
	};

	static void SpMXSpV_ForThreading(const Dcsc<IT,NUM> & Adcsc, int32_t mA, const int32_t * indx, const IVT * numx, int32_t veclen,  
			vector<int32_t> & indy, vector<OVT> & numy, int32_t offset)
	{
		cout << "Threaded version is not yet supported with general (non-boolean) matrices" << endl;
	};
};



template <class SR, class IT, class IVT, class OVT>
struct SpImpl<SR,IT,bool, IVT, OVT>	// specialization
{
	static void SpMXSpV(const Dcsc<IT,bool> & Adcsc, int32_t mA, const int32_t * indx, const IVT * numx, int32_t veclen,  
			vector<int32_t> & indy, vector< OVT > & numy);	

	static void SpMXSpV(const Dcsc<IT,bool> & Adcsc, int32_t mA, const int32_t * indx, const IVT * numx, int32_t veclen,  
			int32_t * indy, OVT * numy, int * cnts, int * dspls, int p_c);

	//! Dcsc and vector index types do not need to match
	static void SpMXSpV_ForThreading(const Dcsc<IT,bool> & Adcsc, int32_t mA, const int32_t * indx, const IVT * numx, int32_t veclen,  
			vector<int32_t> & indy, vector<OVT> & numy, int32_t offset);
};

#include "SpImpl.cpp"
#endif
