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

#ifndef _SP_IMPL_NOSR_H_
#define _SP_IMPL_NOSR_H_

#ifdef INTEGERSORT
#include "PBBS/radixSort.h"
#endif


template <class IT, class NT>
class Dcsc;

template <class IT, class NUM, class VT>
struct SpImplNoSR;

//! Version without the Semiring (for BFS)
template <class IT, class NUM, class VT>
void SpMXSpV(const Dcsc<IT,NUM> & Adcsc, int32_t mA, const int32_t * indx, const VT * numx, int32_t veclen,  
			 int32_t * indy, VT * numy, int * cnts, int * dspls, int p_c, BitMap * isthere)
{
	SpImplNoSR<IT,NUM,VT>::SpMXSpV(Adcsc, mA, indx, numx, veclen, indy, numy, cnts, dspls,p_c, isthere);	// don't touch this
};

template <class IT, class NUM, class VT>
void SpMXSpV_ForThreading(const Dcsc<IT,NUM> & Adcsc, int32_t mA, const int32_t * indx, const VT * numx, int32_t veclen,  
		vector<int32_t> & indy, vector< VT > & numy, int32_t offset)
{
	SpImplNoSR<IT,NUM,VT>::SpMXSpV_ForThreading(Adcsc, mA, indx, numx, veclen, indy, numy, offset);	// don't touch this
};



template <class IT, class NUM, class VT>
struct SpImplNoSR
{
	static void SpMXSpV(const Dcsc<IT,NUM> & Adcsc, int32_t mA, const int32_t * indx, const VT * numx, int32_t veclen,  
						int32_t * indy, VT * numy, int * cnts, int * dspls, int p_c, BitMap * isthere)
	{
		cout << "SpMXSpV (without a semiring) is only reserved for boolean matrices" << endl;
	};

	static void SpMXSpV_ForThreading(const Dcsc<IT,NUM> & Adcsc, int32_t mA, const int32_t * indx, const VT * numx, int32_t veclen,  
			vector<int32_t> & indy, vector<VT> & numy, int32_t offset)
	{
		cout << "SpMXSpV (without a semiring) is only reserved for boolean matrices" << endl;
	};
};


//! Dcsc and vector index types do not need to match
//! However, input and output vector numerical types should be identical
template <class IT, class VT>
struct SpImplNoSR<IT,bool, VT>	// specialization
{
	static void SpMXSpV(const Dcsc<IT,bool> & Adcsc, int32_t mA, const int32_t * indx, const VT * numx, int32_t veclen,  
						int32_t * indy, VT * numy, int * cnts, int * dspls, int p_c, BitMap * isthere);

	static void SpMXSpV_ForThreading(const Dcsc<IT,bool> & Adcsc, int32_t mA, const int32_t * indx, const VT * numx, int32_t veclen,  
			vector<int32_t> & indy, vector<VT> & numy, int32_t offset);
};


/**
 * @param[in,out]   indy,numy,cnts 	{preallocated arrays to be filled}
 * @param[in] 		dspls	{displacements to preallocated indy,numy buffers}
 * This version determines the receiving column neighbor and adjust the indices to the receiver's local index
 * It also by passes-SPA by relying on the fact that x (rhs vector) is sorted and values are indices
 * (If they are not sorted, it'd still work but be non-deterministic)
 * Hence, Semiring operations are not needed (no add or multiply)
 * Also allows the vector's indices to be different than matrix's (for transition only) \TODO: Disable?
 **/
template <typename IT, typename VT>
void SpImplNoSR<IT,bool,VT>::SpMXSpV(const Dcsc<IT,bool> & Adcsc, int32_t mA, const int32_t * indx, const VT * numx, int32_t veclen,  
										   int32_t * indy, VT * numy, int * cnts, int * dspls, int p_c, BitMap * isthere)
{   
	typedef pair<uint32_t,VT>  UPAIR; 
	int32_t perproc = mA / p_c;	
	int32_t k = 0; 	// index to indx vector
	IT i = 0; 	// index to columns of matrix
#ifndef INTEGERSORT
	vector< vector<UPAIR> > nzinds_vals(p_c);	// nonzero indices + associated parent assignments
	while(i< Adcsc.nzc && k < veclen)
	{
		if(Adcsc.jc[i] < indx[k]) ++i;
		else if(indx[k] < Adcsc.jc[i]) ++k;
		else
		{
			for(IT j=Adcsc.cp[i]; j < Adcsc.cp[i+1]; ++j)	// for all nonzeros in this column
			{
				int32_t rowid = (int32_t) Adcsc.ir[j];
				if(!isthere->get_bit(rowid))
				{
					int32_t owner = min(rowid / perproc, static_cast<int32_t>(p_c-1)); 			
					nzinds_vals[owner].push_back( UPAIR(rowid, numx[k]) );
					isthere->set_bit(rowid);
				}	// skip existing entries
			}
			++i; ++k;
		}
	}
	for(int p = 0; p< p_c; ++p)
	{
		sort(nzinds_vals[p].begin(), nzinds_vals[p].end());
		cnts[p] = nzinds_vals[p].size();
		int32_t offset = perproc * p;
		for(int i=0; i< cnts[p]; ++i)
		{
			indy[dspls[p]+i] = nzinds_vals[p][i].first - offset;	// convert to local offset
			numy[dspls[p]+i] = nzinds_vals[p][i].second; 	
		}
	}
#else
	vector< UPAIR* > nzinds_vals(p_c);	// nonzero indices + associated parent assignments
	vector< int > counts(p_c, 0);
	BitMap * tempisthere = new BitMap(*isthere);
	vector<int32_t> matfingers;	// fingers to matrix
	vector<int32_t> vecfingers; // fingers to vector
	while(i< Adcsc.nzc && k < veclen)
	{
		if(Adcsc.jc[i] < indx[k]) ++i;
		else if(indx[k] < Adcsc.jc[i]) ++k;
		else
		{
			matfingers.push_back(i);
			vecfingers.push_back(k);
			for(IT j=Adcsc.cp[i]; j < Adcsc.cp[i+1]; ++j)	// for all nonzeros in this column
			{
				int32_t rowid = (int32_t) Adcsc.ir[j];
				if(!tempisthere->get_bit(rowid))
				{
					int32_t owner = min(rowid / perproc, static_cast<int32_t>(p_c-1)); 			
					++counts[owner];
					tempisthere->set_bit(rowid);
				}
			}
			++i;	++k;
		}
	}
	delete tempisthere;
	for(int p=0; p< p_c; ++p)
		nzinds_vals[p] = (UPAIR*) malloc(sizeof(UPAIR) * counts[p]);
	
	fill(counts.begin(), counts.end(), 0);	// reset counts
	int fsize = matfingers.size();
	for(int i=0; i< fsize; ++i)
	{
		int colfin = matfingers[i];	// column finger
		for(IT j=Adcsc.cp[colfin]; j < Adcsc.cp[colfin+1]; ++j)	// for all nonzeros in this column
		{
			int32_t rowid = (int32_t) Adcsc.ir[j];
			if(!isthere->get_bit(rowid))
			{
				int32_t owner = min(rowid / perproc, static_cast<int32_t>(p_c-1)); 			
				nzinds_vals[owner][counts[owner]++] =  UPAIR(rowid, numx[vecfingers[i]]) ;
				isthere->set_bit(rowid);
			}
		}
	}

	for(int p = 0; p< p_c; ++p)
	{
		integerSort(nzinds_vals[p] , counts[p]);
		int oldcnt = cnts[p];
		cnts[p] += counts[p];
		int32_t offset = perproc * p;
		for(int i=oldcnt; i< cnts[p]; ++i)
		{
			indy[dspls[p]+i] = nzinds_vals[p][i-oldcnt].first - offset;	// convert to local offset
			numy[dspls[p]+i] = nzinds_vals[p][i-oldcnt].second; 	
		}
		free(nzinds_vals[p]);
	}
#endif
}


// BFS only version without any semiring parameters 
template <typename IT, typename VT>
void SpImplNoSR<IT,bool,VT>::SpMXSpV_ForThreading(const Dcsc<IT,bool> & Adcsc, int32_t mA, const int32_t * indx, const VT * numx, int32_t veclen,  
			vector<int32_t> & indy, vector<VT> & numy, int32_t offset)
{   
	VT * localy = new VT[mA];
	bool * isthere = new bool[mA];
	fill(isthere, isthere+mA, false);
	vector<int32_t> nzinds;	// nonzero indices		

	// The following piece of code is not general, but it's more memory efficient than FillColInds
	int32_t k = 0; 	// index to indx vector
	IT i = 0; 	// index to columns of matrix
	while(i< Adcsc.nzc && k < veclen)
	{
		if(Adcsc.jc[i] < indx[k]) ++i;
		else if(indx[k] < Adcsc.jc[i]) ++k;
		else
		{
			for(IT j=Adcsc.cp[i]; j < Adcsc.cp[i+1]; ++j)	// for all nonzeros in this column
			{
				int32_t rowid = (int32_t) Adcsc.ir[j];
				if(!isthere[rowid])
				{
					localy[rowid] = numx[k];	// initial assignment
					nzinds.push_back(rowid);
					isthere[rowid] = true;
				}
				// skip existing entries
			}
			++i;
			++k;
		}
	}

	sort(nzinds.begin(), nzinds.end());
	int nnzy = nzinds.size();
	indy.resize(nnzy);
	numy.resize(nnzy);
	for(int i=0; i< nnzy; ++i)
	{
		indy[i] = nzinds[i] + offset;	// return column-global index and let gespmv determine the receiver's local index
		numy[i] = localy[nzinds[i]]; 	
	}
	delete [] localy;
	delete [] isthere;
}


#endif
