#ifndef _LOC_ARR_H_
#define _LOC_ARR_H_


template<class V, class C>
struct LocArr
{
	LocArr():addr(NULL),count(0) {}
	LocArr(V * myaddr, C mycount): addr(myaddr ), count(mycount){}
	
	V * addr;
	C count;
};

template<class IT, class NT>
struct Arr
{
	Arr(IT indsize, IT numsize) 
	{ 
		indarrs.resize(indsize);
		numarrs.resize(numsize);
	} 

	vector< LocArr<IT,IT> > indarrs;
	vector< LocArr<NT,IT> > numarrs;

	IT totalsize() { return indarrs.size() + numarrs.size(); } 	
};

#endif

