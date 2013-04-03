#ifndef _HEAP_ENTRY_H
#define _HEAP_ENTRY_H

template <class IT, class NT>
class HeapEntry
{
public:
	HeapEntry() {};
	HeapEntry(IT mykey, IT myrunr, NT mynum): key(mykey), runr(myrunr),num(mynum){}
	IT key;	
	IT runr;	 
	NT num;	

	// Operators are swapped for performance
	// If you want/need to convert them back to their normal definitions, don't forget to add
	// "greater< HeapEntry<T> >()" optional parameter to all the heap operations operating on HeapEntry<T> objects.
	// For example: push_heap(heap, heap + kisect, greater< HeapEntry<T> >());

	bool operator > (const HeapEntry & rhs) const
	{ return (key < rhs.key); }
	bool operator < (const HeapEntry & rhs) const
	{ return (key > rhs.key); }
	bool operator == (const HeapEntry & rhs) const
	{ return (key == rhs.key); }
};

#endif

