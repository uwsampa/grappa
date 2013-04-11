#ifndef _OPT_BUF_H
#define _OPT_BUF_H
#include "BitMap.h"

/**
  * This special data structure is used for optimizing BFS iterations
  * by providing a fixed sized buffer for communication
  * the contents of the buffer are irrelevant until SpImpl:SpMXSpV starts
  * hence the copy constructor that doesn't copy contents
  */
template <class IT, class NT>
class OptBuf
{
public:
	OptBuf(): isthere(NULL), p_c(0), totmax(0), localm(0) {};
	void MarkEmpty()
	{
		if(totmax > 0)
		{
		//	fill(isthere, isthere+localm, false);
			isthere->reset();
		}
	}
	
	void Set(const vector<int> & maxsizes, int mA) 
	{
		p_c =  maxsizes.size(); 
		totmax = accumulate(maxsizes.begin(), maxsizes.end(), 0);
		inds = new IT[totmax];
		fill_n(inds, totmax, -1);
		nums = new NT[totmax];
		dspls = new int[p_c]();
		partial_sum(maxsizes.begin(), maxsizes.end()-1, dspls+1);
		localm = mA;
		//isthere = new bool[localm];
		//fill(isthere, isthere+localm, false);
		isthere = new BitMap(localm);
	};
	~OptBuf()
	{	if(localm > 0)
		{
			//delete [] isthere;
			delete isthere;
		}
		
		if(totmax > 0)
		{
			delete [] inds;
			delete [] nums;
		}
		if(p_c > 0)
			delete [] dspls;
	}
	OptBuf(const OptBuf<IT,NT> & rhs)
	{
		p_c = rhs.p_c;
		totmax = rhs.totmax;
		localm = rhs.localm;
		inds = new IT[totmax];
		nums = new NT[totmax];
		dspls = new int[p_c]();	
		//isthere = new bool[localm];
		//fill(isthere, isthere+localm, false);
		isthere = new BitMap(localm);
	}
	OptBuf<IT,NT> & operator=(const OptBuf<IT,NT> & rhs)
	{
		if(this != &rhs)
		{
			if(localm > 0)
			{
				//delete [] isthere;
				delete isthere;
			}
			if(totmax > 0)
			{
				delete [] inds;
				delete [] nums;
			}
			if(p_c > 0)
				delete [] dspls;
	
			p_c = rhs.p_c;
			totmax = rhs.totmax;
			localm = rhs.localm;
			inds = new IT[totmax];
			nums = new NT[totmax];
			dspls = new int[p_c]();	
			isthere = new BitMap(*(rhs.isthere));
		}
		return *this;
	}
	
	IT * inds;	
	NT * nums;	
	int * dspls;
	//bool * isthere;
	BitMap * isthere;
	int p_c;
	int totmax;
	int localm;
};

#endif

