#include "MemoryPool.h"

MemoryPool::MemoryPool(void * m_beg, size_t m_size):initbeg((char*)m_beg), initend(((char*)m_beg)+m_size)
{
	Memory m((char*) m_beg, m_size);
	freelist.push_back(m);
}

void * MemoryPool::alloc(size_t size)
{
	for(list<Memory>::iterator iter = freelist.begin(); iter != freelist.end(); ++iter)
	{
    	if ((*iter).size > size)	// return the first 'big enough' chunk of memory
		{
			char * free = (*iter).begin;
			(*iter).begin += size;		// modify the beginning of the remaining chunk
			(*iter).size -= size;		// modify the size of the remaning chunk

			return (void *) free;		// return the memory 
		}
	}
	cout << "No pinned memory available" << endl;
	return NULL;
}


void MemoryPool::dealloc(void * base, size_t size)
{
	if( ((char*) base) >= initbeg && (((char*)base) + size) < initend)
	{	
		list<Memory>::iterator titr = freelist.begin();	// trailing iterator
		list<Memory>::iterator litr = freelist.begin();	// leading iterator
		++litr;
		
		if( (char*)base < titr->begaddr()) 	// if we're inserting to the front of the list
		{
			if(titr->begaddr() == ((char*)base) + size)
			{
				titr->begin = (char*)base;
				titr->size += size;
			}
			else
			{
				Memory m((char*) base, size);
				freelist.insert(titr, m);
			}
		}
		else if( litr == freelist.end() )	// if we're inserting to the end of a 'single element list'
		{
			if(titr->endaddr() == (char*)base)
			{
				titr->size += size;
			}
			else
			{
				Memory m((char*) base, size);
				freelist.insert(litr, m);
			}	
		}
		else		// not a single element list, nor we're inserting to the front
		{
			// loop until you find the right spot
			while( litr != freelist.end() && litr->begaddr() < (char*)base)
			{  ++litr; 	++titr;	}

			//! defragment on the fly 
			//! by attaching to the previous chunk if prevchunk.endaddr equals newitem.beginaddr, or
			//! by attaching to the next available chunk if newitem.endaddr equals nextchunk.begaddr 	
			if(titr->endaddr() == (char*)base)
			{
				//! check the next chunk to see if we perfectly fill the hole 
				if( litr == freelist.end() || litr->begaddr() != ((char*)base) + size)
				{
					titr->size += size;
				}
				else
				{
					titr->size  += (size + litr->size);	// modify the chunk pointed by trailing iterator
					freelist.erase(litr); 			// delete the chunk pointed by the leading iterator
				}
			}
			else
			{
				if( litr == freelist.end() || litr->begaddr() != ((char*)base) + size)
				{
					Memory m((char*) base, size);
				
					//! Insert x before pos: 'iterator insert(iterator pos, const T& x)'
					freelist.insert(litr, m);					
				}
				else
				{
					litr->begin = (char*)base;
					litr->size += size;
				}					
			}
		}
	}
	else
	{
		cerr << "Memory starting at " << base << " and ending at " 
		<< (void*) ((char*) base + size) << " is out of pool bounds, cannot dealloc()" << endl;
	}
}

//! Dump the contents of the pinned memory
ofstream& operator<< (ofstream& outfile, const MemoryPool & mpool)
{
	int i = 0;
	for(list<Memory>::const_iterator iter = mpool.freelist.begin(); iter != mpool.freelist.end(); ++iter, ++i)
	{
		outfile << "Chunk " << i << " of size: " << (*iter).size << " starts:" <<  (void*)(*iter).begin 
			<< " and ends: " << (void*) ((*iter).begin + (*iter).size) << endl ; 
	}
	return outfile;
}

