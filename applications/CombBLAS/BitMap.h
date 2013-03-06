/****************************************************************/
/* Parallel Combinatorial BLAS Library (for Graph Computations) */
/* version 1.3 -------------------------------------------------*/
/* date: 05/01/2012 --------------------------------------------*/
/* authors: Aydin Buluc (abuluc@lbl.gov), Adam Lugowski --------*/
/* this file contributed by Scott Beamer of UC Berkeley --------*/
/****************************************************************/
/*
 Copyright (c) 2012, Scott Beamer
 
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

#ifndef BITMAP_H
#define BITMAP_H

#include <stdint.h>

#define WORD_OFFSET(n) (n/64)
#define BIT_OFFSET(n) (n & 0x3f)


class BitMap {
 public:
  // actually always rounds up
  BitMap() { start = NULL; end = NULL;};	// default constructor

  BitMap(uint64_t size) {
    uint64_t num_longs = (size + 63) / 64;
    // VS9:  warning C4244: 'initializing' : conversion from 'uint64_t' to 'unsigned int', possible loss of data
    start = new uint64_t[num_longs];
    end = start + num_longs;
  }

  ~BitMap() {
    delete[] start;
  }
  BitMap(const BitMap & rhs)
  {
	uint64_t num_longs = rhs.end - rhs.start;
	// VS9: warning C4244: 'initializing' : conversion from 'uint64_t' to 'unsigned int', possible loss of data
	start = new uint64_t[num_longs];
	end = start + num_longs;
	copy(rhs.start, rhs.end, start);
  }
  BitMap & operator= (const BitMap & rhs)
  {
	if(this != &rhs)
        {
		delete [] start;
		uint64_t num_longs = rhs.end - rhs.start;
		// VS9: warning C4244: 'initializing' : conversion from 'uint64_t' to 'unsigned int', possible loss of data
        	start = new uint64_t[num_longs];
        	end = start + num_longs;
        	copy(rhs.start, rhs.end, start);
	}
	return *this;
   }
    

  inline
  void reset() {
    for(uint64_t *it=start; it!=end; it++)
      *it = 0;
  }

  inline
  void set_bit(uint64_t pos) {
    start[WORD_OFFSET(pos)] |= ( static_cast<uint64_t>(1l)<<BIT_OFFSET(pos));
  }

  inline
  void set_bit_atomic(long pos) {
    set_bit(pos);
    // uint64_t old_val, new_val;
    // uint64_t *loc = start + WORD_OFFSET(pos);
    // do {
    //   old_val = *loc;
    //   new_val = old_val | ((uint64_t) 1l<<BIT_OFFSET(pos));
    // } while(!__sync_bool_compare_and_swap(loc, old_val, new_val));
  }

  inline
  bool get_bit(uint64_t pos) {
// VS9: warning C4334: '<<' : result of 32-bit shift implicitly converted to 64 bits (was 64-bit shift intended?)
    if (start[WORD_OFFSET(pos)] & ( static_cast<uint64_t>(1l) <<BIT_OFFSET(pos)))
      return true;
    else
      return false;
  }

  inline
  long get_next_bit(uint64_t pos) {
    uint64_t next = pos;
    int bit_offset = BIT_OFFSET(pos);
    uint64_t *it = start + WORD_OFFSET(pos);
    uint64_t temp = (*it);
    if (bit_offset != 63) {
      temp = temp >> (bit_offset+1);
    } else {
      temp = 0;
    }
    if (!temp) {
      next = (next & 0xffffffc0);
      while (!temp) {
        it++;
        if (it >= end)
          return -1;
        temp = *it;
        next += 64;
      }
    } else {
      next++;
    }
    while(!(temp&1)) {
      temp = temp >> 1;
      next++;
    }
    return next;
  }

  inline
  uint64_t* data() {
    return start;
  }

  void print_ones() {
    /* 
    uint64_t max_size = (end-start)*64;
    for (uint64_t i=0; i<max_size; i++)
      if (get_bit(i))
        cout << " " << i;
    cout << endl;
    */ 
  }

 private:
  uint64_t *start;
  uint64_t *end;
};

#endif // BITMAP_H
