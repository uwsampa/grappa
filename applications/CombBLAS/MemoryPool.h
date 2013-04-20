/****************************************************************/
/* Sequential and Parallel Sparse Matrix Multiplication Library  /
/  version 2.3 --------------------------------------------------/
/  date: 01/18/2009 ---------------------------------------------/
/  description: Simple memory pool implementation ---------------/
/  author: Aydin Buluc (aydin@cs.ucsb.edu) ----------------------/
/  This can be improved by keeping a second index that keeps ----/
/  track of order in terms of size, so that finding an available /
/  sized chunk is optimal and fast ------------------------------/
\****************************************************************/

#ifndef _MEMORY_POOL_H
#define _MEMORY_POOL_H

#include <iostream>
#include <list>
#include <new>			// For "placement new" (classes using this memory pool may need it)
#include <fstream>

using namespace std;

//! EqualityComparable memory chunk object
//! Two memory chunks are considered equal if either their beginaddr or the endaddr are the same (just to facilitate deallocation)
class Memory
{
	public:
	Memory(char * m_beg, size_t m_size): begin(m_beg),size(m_size)
	{};
	
	char * begaddr() { return begin; }
	char * endaddr() { return begin + size; }

	bool operator < (const Memory & rhs) const
	{ return (begin < rhs.begin); }
	bool operator == (const Memory & rhs) const
	{ return (begin == rhs.begin); }
	
	char * begin;
	size_t size;
};


/**
  * \invariant Available memory addresses will be sorted w.r.t. their starting positions
  * \invariant At least one element exists in the freelist at any time.
  * \invariant Defragment on the fly: at any time, NO two consecutive chunks with chunk1.endaddr equals chunk2.begaddr exist
  */ 
class MemoryPool
{
public:
	MemoryPool(void * m_beg, size_t m_size);

	void * alloc(size_t size);
	void dealloc (void * base, size_t size);

	friend ofstream& operator<< (ofstream& outfile, const MemoryPool & mpool);	

private:
	// std::list is a doubly linked list (i.e., a Sequence that supports both forward and backward traversal)
	list<Memory> freelist;
	char * initbeg;
	char * initend; 
};


#endif
