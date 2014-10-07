/**  
 * Copyright (c) 2009 Carnegie Mellon University. 
 *     All rights reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an "AS
 *  IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 *  express or implied.  See the License for the specific language
 *  governing permissions and limitations under the License.
 *
 * For more about this software visit:
 *
 *      http://www.graphlab.ml.cmu.edu
 *
 */

/////////////////////////////////////////////////////////////////////
/// This code is directly borrowed from the GraphLab source code in 
/// order to compare lower-level runtime components fairly. It 
/// carries the license and copyright above rather than Grappa's.
/////////////////////////////////////////////////////////////////////


// Jenkin's 32 bit integer mix from
// http://burtleburtle.net/bob/hash/integer.html
inline uint32_t integer_mix(uint32_t a) {
  a -= (a<<6);
  a ^= (a>>17);
  a -= (a<<9);
  a ^= (a<<4);
  a -= (a<<3);
  a ^= (a<<10);
  a ^= (a>>15);
  return a;
}

/** \brief Returns the hashed value of an edge. */
inline static size_t hash_edge(const std::pair<VertexID, VertexID>& e, const uint32_t seed = 5) {
  // a bunch of random numbers
#if (__SIZEOF_PTRDIFF_T__ == 8)
  static const size_t a[8] = {0x6306AA9DFC13C8E7,
    0xA8CD7FBCA2A9FFD4,
    0x40D341EB597ECDDC,
    0x99CFA1168AF8DA7E,
    0x7C55BCC3AF531D42,
    0x1BC49DB0842A21DD,
    0x2181F03B1DEE299F,
    0xD524D92CBFEC63E9};
#else
  static const size_t a[8] = {0xFC13C8E7,
    0xA2A9FFD4,
    0x597ECDDC,
    0x8AF8DA7E,
    0xAF531D42,
    0x842A21DD,
    0x1DEE299F,
    0xBFEC63E9};
#endif
  VertexID src = e.first;
  VertexID dst = e.second;
  return (integer_mix(src^a[seed%8]))^(integer_mix(dst^a[(seed+1)%8]));
}

//////////////////////////////////////////////////////////////////
// graphlab: dense_bitset.hpp
//////////////////////////////////////////////////////////////////
/**  
 * Copyright (c) 2009 Carnegie Mellon University. 
 *     All rights reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an "AS
 *  IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 *  express or implied.  See the License for the specific language
 *  governing permissions and limitations under the License.
 *
 * For more about this software visit:
 *
 *      http://www.graphlab.ml.cmu.edu
 *
 */

#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdint.h>

namespace graphlab {
  /**
   * \ingroup util
     atomic instruction that is equivalent to the following:
     \code
     if (a==oldval) {    
       a = newval;           
       return true;          
     }
     else {
       return false;
    }
    \endcode
  */
  template<typename T>
  bool atomic_compare_and_swap(T& a, T oldval, T newval) {
    return __sync_bool_compare_and_swap(&a, oldval, newval);
  };

  /**
   * \ingroup util
     atomic instruction that is equivalent to the following:
     \code
     if (a==oldval) {    
       a = newval;           
       return true;          
     }
     else {
       return false;
    }
    \endcode
  */
  template<typename T>
  bool atomic_compare_and_swap(volatile T& a, 
                               T oldval, 
                               T newval) {
    return __sync_bool_compare_and_swap(&a, oldval, newval);
  };

  /**
   * \ingroup util
     atomic instruction that is equivalent to the following:
     \code
     if (a==oldval) {    
       a = newval;           
       return true;          
     }
     else {
       return false;
    }
    \endcode
  */
  template <>
  inline bool atomic_compare_and_swap(volatile double& a, 
                                      double oldval, 
                                      double newval) {
    volatile uint64_t* a_ptr = reinterpret_cast<volatile uint64_t*>(&a);
    const uint64_t* oldval_ptr = reinterpret_cast<const uint64_t*>(&oldval);
    const uint64_t* newval_ptr = reinterpret_cast<const uint64_t*>(&newval);
    return __sync_bool_compare_and_swap(a_ptr, *oldval_ptr, *newval_ptr);
  };

  /**
   * \ingroup util
     atomic instruction that is equivalent to the following:
     \code
     if (a==oldval) {    
       a = newval;           
       return true;          
     }
     else {
       return false;
    }
    \endcode
  */
  template <>
  inline bool atomic_compare_and_swap(volatile float& a, 
                                      float oldval, 
                                      float newval) {
    volatile uint32_t* a_ptr = reinterpret_cast<volatile uint32_t*>(&a);
    const uint32_t* oldval_ptr = reinterpret_cast<const uint32_t*>(&oldval);
    const uint32_t* newval_ptr = reinterpret_cast<const uint32_t*>(&newval);
    return __sync_bool_compare_and_swap(a_ptr, *oldval_ptr, *newval_ptr);
  };

  /** 
    * \ingroup util
    * \brief Atomically exchanges the values of a and b.
    * \warning This is not a full atomic exchange. Read of a,
    * and the write of b into a is atomic. But the write into b is not.
    */
  template<typename T>
  void atomic_exchange(T& a, T& b) {
    b = __sync_lock_test_and_set(&a, b);
  };

  /** 
    * \ingroup util
    * \brief Atomically exchanges the values of a and b.
    * \warning This is not a full atomic exchange. Read of a,
    * and the write of b into a is atomic. But the write into b is not.
    */
  template<typename T>
  void atomic_exchange(volatile T& a, T& b) {
    b = __sync_lock_test_and_set(&a, b);
  };

  /** 
    * \ingroup util
    * \brief Atomically sets a to the newval, returning the old value
    */
  template<typename T>
  T fetch_and_store(T& a, const T& newval) {
    return __sync_lock_test_and_set(&a, newval);
  };

}

namespace graphlab {
  
  /**  \ingroup util
   *  Implements an atomic dense bitset
   */
  class dense_bitset {
  public:
    
    /// Constructs a bitset of 0 length
    dense_bitset() : array(NULL), len(0), arrlen(0) {
    }

    /// Constructs a bitset with 'size' bits. All bits will be cleared.
    explicit dense_bitset(size_t size) : array(NULL), len(0), arrlen(0) {
      resize(size);
      clear();
    }

    /// Make a copy of the bitset db
    dense_bitset(const dense_bitset &db) {
      array = NULL;
      len = 0;
      arrlen = 0;
      *this = db;
    }
    
    /// destructor
    ~dense_bitset() {free(array);}
  
    /// Make a copy of the bitset db
    inline dense_bitset& operator=(const dense_bitset& db) {
      resize(db.size());
      len = db.len;
      arrlen = db.arrlen;
      memcpy(array, db.array, sizeof(size_t) * arrlen);
      return *this;
    }
  
    /** Resizes the current bitset to hold n bits.
    Existing bits will not be changed. If the array size is increased,
    the value of the new bits are undefined.
    
    \Warning When shirnking, the current implementation may still leave the
    "deleted" bits in place which will mess up the popcount. 
    */
    inline void resize(size_t n) {
      len = n;
      //need len bits
      size_t prev_arrlen = arrlen;
      arrlen = (n / (sizeof(size_t) * 8)) + (n % (sizeof(size_t) * 8) > 0);
      array = (size_t*)realloc(array, sizeof(size_t) * arrlen);
      // this zeros the remainder of the block after the last bit
      fix_trailing_bits();
      // if we grew, we need to zero all new blocks
      if (arrlen > prev_arrlen) {
        for (size_t i = prev_arrlen; i < arrlen; ++i) {
          array[i] = 0;
        }
      }
    }
  
    /// Sets all bits to 0
    inline void clear() {
      for (size_t i = 0; i < arrlen; ++i) array[i] = 0;
    }
    
    inline bool empty() const {
      for (size_t i = 0; i < arrlen; ++i) if (array[i]) return false;
      return true;
    }
    
    /// Sets all bits to 1
    inline void fill() {
      for (size_t i = 0;i < arrlen; ++i) array[i] = (size_t) - 1;
      fix_trailing_bits();
    }

    /// Prefetches the word containing the bit b
    inline void prefetch(size_t b) const{
      __builtin_prefetch(&(array[b / (8 * sizeof(size_t))]));
    }
    
    /// Returns the value of the bit b
    inline bool get(size_t b) const{
      size_t arrpos, bitpos;
      bit_to_pos(b, arrpos, bitpos);
      return array[arrpos] & (size_t(1) << size_t(bitpos));
    }

    //! Atomically sets the bit at position b to true returning the old value
    inline bool set_bit(size_t b) {
      // use CAS to set the bit
      size_t arrpos, bitpos;
      bit_to_pos(b, arrpos, bitpos);
      const size_t mask(size_t(1) << size_t(bitpos)); 
      return __sync_fetch_and_or(array + arrpos, mask) & mask;
    }
    
    //! Atomically xors a bit with 1
    inline bool xor_bit(size_t b) {
      // use CAS to set the bit
      size_t arrpos, bitpos;
      bit_to_pos(b, arrpos, bitpos);
      const size_t mask(size_t(1) << size_t(bitpos)); 
      return __sync_fetch_and_xor(array + arrpos, mask) & mask;
    }
 
    //! Returns the value of the word containing the bit b 
    inline size_t containing_word(size_t b) {
      size_t arrpos, bitpos;
      bit_to_pos(b, arrpos, bitpos);
      return array[arrpos];
    }

    //! Returns the value of the word containing the bit b 
    inline size_t get_containing_word_and_zero(size_t b) {
      size_t arrpos, bitpos;
      bit_to_pos(b, arrpos, bitpos);
      return fetch_and_store(array[arrpos], size_t(0));
    }

    /** 
     * \brief Transfers approximately b bits from another bitset to this bitset 
     * 
     * "Moves" at least b bits from the other bitset to this bitset
     * starting from the given position.
     * At return, b will contain the actual number of bits moved,
     * and start will point to the end of the transfered region.
     *
     * Semantically what this accomplishes is something like:
     *
     * \code
     * idx = start;
     * if other.get_bit(idx) == false {
     *    idx = next true bit after idx in other (with loop around)
     * }
     * for(transferred = 0; transferred < b; transferred++) {
     *    other.clear_bit(idx);
     *    this->set_bit(idx);
     *    idx = next true bit after idx in other.
     *    if no more bits, return
     * }
     * \endcode
     * However, the implementation here may transfer more than b bits.
     * ( up to b + 2 * wordsize_in_bits )
     */
    inline void transfer_approximate_unsafe(dense_bitset& other, 
                                            size_t& start, 
                                            size_t& b) {
      // must be identical in length
      CHECK_EQ(other.len, len);
      CHECK_EQ(other.arrlen, arrlen);
      size_t arrpos, bitpos;
      bit_to_pos(start, arrpos, bitpos);
      size_t initial_arrpos = arrpos;
      if (arrpos >= arrlen) arrpos = 0;
      // ok. we will only look at arrpos
      size_t transferred = 0;
      while(transferred < b) {
        if (other.array[arrpos] > 0) { 
          transferred += __builtin_popcountl(other.array[arrpos]);
          array[arrpos] |= other.array[arrpos];
          other.array[arrpos] = 0;
        }
        ++arrpos;
        if (arrpos >= other.arrlen) arrpos = 0;
        else if (arrpos == initial_arrpos) break;
      }
      start = 8 * sizeof(size_t) * arrpos;
      b = transferred;
    }


    /** Set the bit at position b to true returning the old value.
        Unlike set_bit(), this uses a non-atomic set which is faster,
        but is unsafe if accessed by multiple threads.
    */
    inline bool set_bit_unsync(size_t b) {
      // use CAS to set the bit
      size_t arrpos, bitpos;
      bit_to_pos(b, arrpos, bitpos);
      const size_t mask(size_t(1) << size_t(bitpos)); 
      bool ret = array[arrpos] & mask;
      array[arrpos] |= mask;
      return ret;
    }

    //! Atomically sets the state of the bit to the new value returning the old value
    inline bool set(size_t b, bool value) {
      if (value) return set_bit(b);
      else return clear_bit(b);
    }

    /** Set the state of the bit returning the old value.
      This version uses a non-atomic set which is faster, but
      is unsafe if accessed by multiple threads.
    */
    inline bool set_unsync(size_t b, bool value) {
      if (value) return set_bit_unsync(b);
      else return clear_bit_unsync(b);
    }


    //! Atomically set the bit at b to false returning the old value
    inline bool clear_bit(size_t b) {
      // use CAS to set the bit
      size_t arrpos, bitpos;
      bit_to_pos(b, arrpos, bitpos);
      const size_t test_mask(size_t(1) << size_t(bitpos)); 
      const size_t clear_mask(~test_mask); 
      return __sync_fetch_and_and(array + arrpos, clear_mask) & test_mask;
    }

    /** Clears the state of the bit returning the old value.
      This version uses a non-atomic set which is faster, but
      is unsafe if accessed by multiple threads.
    */
    inline bool clear_bit_unsync(size_t b) {
      // use CAS to set the bit
      size_t arrpos, bitpos;
      bit_to_pos(b, arrpos, bitpos);
      const size_t test_mask(size_t(1) << size_t(bitpos)); 
      const size_t clear_mask(~test_mask); 
      bool ret = array[arrpos] & test_mask;
      array[arrpos] &= clear_mask;
      return ret;
    }

    struct bit_pos_iterator {
      typedef std::input_iterator_tag iterator_category;
      typedef size_t value_type;
      typedef size_t difference_type;
      typedef const size_t reference;
      typedef const size_t* pointer;
      size_t pos;
      const dense_bitset* db;
      bit_pos_iterator():pos(-1),db(NULL) {}
      bit_pos_iterator(const dense_bitset* const db, size_t pos):pos(pos),db(db) {}
      
      size_t operator*() const {
        return pos;
      }
      size_t operator++(){
        if (db->next_bit(pos) == false) pos = (size_t)(-1);
        return pos;
      }
      size_t operator++(int){
        size_t prevpos = pos;
        if (db->next_bit(pos) == false) pos = (size_t)(-1);
        return prevpos;
      }
      bool operator==(const bit_pos_iterator& other) const {
        CHECK(db == other.db);
        return other.pos == pos;
      }
      bool operator!=(const bit_pos_iterator& other) const {
        CHECK(db == other.db);
        return other.pos != pos;
      }
    };
    
    typedef bit_pos_iterator iterator;
    typedef bit_pos_iterator const_iterator;

    
    bit_pos_iterator begin() const {
      size_t pos;
      if (first_bit(pos) == false) pos = size_t(-1);
      return bit_pos_iterator(this, pos);
    }
    
    bit_pos_iterator end() const {
      return bit_pos_iterator(this, (size_t)(-1));
    }

    /** Returns true with b containing the position of the 
        first bit set to true.
        If such a bit does not exist, this function returns false.
    */
    inline bool first_bit(size_t &b) const {
      for (size_t i = 0; i < arrlen; ++i) {
        if (array[i]) {
          b = (size_t)(i * (sizeof(size_t) * 8)) + first_bit_in_block(array[i]);
          return true;
        }
      }
      return false;
    }


    /** Returns true with b containing the position of the 
        first bit set to false.
        If such a bit does not exist, this function returns false.
    */
    inline bool first_zero_bit(size_t &b) const {
      for (size_t i = 0; i < arrlen; ++i) {
        if (~array[i]) {
          b = (size_t)(i * (sizeof(size_t) * 8)) + first_bit_in_block(~array[i]);
          return true;
        }
      }
      return false;
    }

    /** Where b is a bit index, this function will return in b,
        the position of the next bit set to true, and return true.
        If all bits after b are false, this function returns false.
    */
    inline bool next_bit(size_t &b) const {
      size_t arrpos, bitpos;
      bit_to_pos(b, arrpos, bitpos);
      //try to find the next bit in this block
      bitpos = next_bit_in_block(bitpos, array[arrpos]);
      if (bitpos != 0) {
        b = (size_t)(arrpos * (sizeof(size_t) * 8)) + bitpos;
        return true;
      }
      else {
        // we have to loop through the rest of the array
        for (size_t i = arrpos + 1; i < arrlen; ++i) {
          if (array[i]) {
            b = (size_t)(i * (sizeof(size_t) * 8)) + first_bit_in_block(array[i]);
            return true;
          }
        }
      }
      return false;
    }

    ///  Returns the number of bits in this bitset
    inline size_t size() const {
      return len;
    }
    
    // /// Serializes this bitset to an archive
    // inline void save(oarchive& oarc) const {
    //   oarc <<len << arrlen;
    //   if (arrlen > 0) serialize(oarc, array, arrlen*sizeof(size_t));
    // }
    // 
    // /// Deserializes this bitset from an archive
    // inline void load(iarchive& iarc) {
    //   if (array != NULL) free(array);
    //   array = NULL;
    //   iarc >> len >> arrlen;
    //   if (arrlen > 0) {
    //     array = (size_t*)malloc(arrlen*sizeof(size_t));
    //     deserialize(iarc, array, arrlen*sizeof(size_t));
    //   }
    // }
    // 

    size_t popcount() const {
      size_t ret = 0;
      for (size_t i = 0;i < arrlen; ++i) {
        ret +=  __builtin_popcountl(array[i]);
      }
      return ret;
    }

    dense_bitset operator&(const dense_bitset& other) const {
      CHECK_EQ(size(), other.size());
      dense_bitset ret(size());
      for (size_t i = 0; i < arrlen; ++i) {
        ret.array[i] = array[i] & other.array[i];
      }
      return ret;
    }


    dense_bitset operator|(const dense_bitset& other) const {
      CHECK_EQ(size(), other.size());
      dense_bitset ret(size());
      for (size_t i = 0; i < arrlen; ++i) {
        ret.array[i] = array[i] | other.array[i];
      }
      return ret;
    }

    dense_bitset operator-(const dense_bitset& other) const {
      CHECK_EQ(size(), other.size());
      dense_bitset ret(size());
      for (size_t i = 0; i < arrlen; ++i) {
        ret.array[i] = array[i] - (array[i] & other.array[i]);
      }
      return ret;
    }


    dense_bitset& operator&=(const dense_bitset& other) {
      CHECK_EQ(size(), other.size());
      for (size_t i = 0; i < arrlen; ++i) {
        array[i] &= other.array[i];
      }
      return *this;
    }


    dense_bitset& operator|=(const dense_bitset& other) {
      CHECK_EQ(size(), other.size());
      for (size_t i = 0; i < arrlen; ++i) {
        array[i] |= other.array[i];
      }
      return *this;
    }

    dense_bitset& operator-=(const dense_bitset& other) {
      CHECK_EQ(size(), other.size());
      for (size_t i = 0; i < arrlen; ++i) {
        array[i] = array[i] - (array[i] & other.array[i]);
      }
      return *this;
    }

    void invert() {
      for (size_t i = 0; i < arrlen; ++i) {
        array[i] = ~array[i];
      }
      fix_trailing_bits();
    }

  private:
   
    inline static void bit_to_pos(size_t b, size_t& arrpos, size_t& bitpos) {
      // the compiler better optimize this...
      arrpos = b / (8 * sizeof(size_t));
      bitpos = b & (8 * sizeof(size_t) - 1);
    }
  
    // returns 0 on failure
    inline size_t next_bit_in_block(const size_t& b, const size_t& block) const {
      size_t belowselectedbit = size_t(-1) - (((size_t(1) << b) - 1)|(size_t(1)<<b));
      size_t x = block & belowselectedbit ;
      if (x == 0) return 0;
      else return (size_t)__builtin_ctzl(x);
    }

    // returns 0 on failure
    inline size_t first_bit_in_block(const size_t& block) const{
      if (block == 0) return 0;
      else return (size_t)__builtin_ctzl(block);
    }


    void fix_trailing_bits() {
      // how many bits are in the last block
      size_t lastbits = len % (8 * sizeof(size_t));
      if (lastbits == 0) return;
      array[arrlen - 1] &= ((size_t(1) << lastbits) - 1);
    }

    size_t* array;
    size_t len;
    size_t arrlen;

    template <int len>
    friend class fixed_dense_bitset;
  };
























  
  
  /**
  Like bitset, but of a fixed length as defined by the template parameter
  */
  template <int len>
  class fixed_dense_bitset {
  public:
    /// Constructs a bitset of 0 length
    fixed_dense_bitset() {
      clear();
    }
    
   /// Make a copy of the bitset db
    fixed_dense_bitset(const fixed_dense_bitset<len> &db) {
      *this = db;
    }

    /** Initialize this fixed dense bitset by copying 
        ceil(len/(wordlen)) words from mem
    */
    void initialize_from_mem(void* mem, size_t memlen) {
      memcpy(array, mem, memlen);
    }
    
    /// destructor
    ~fixed_dense_bitset() {}
  
    /// Make a copy of the bitset db
    inline fixed_dense_bitset<len>& operator=(const fixed_dense_bitset<len>& db) {
      memcpy(array, db.array, sizeof(size_t) * arrlen);
      return *this;
    }
  
    /// Sets all bits to 0
    inline void clear() {
      memset((void*)array, 0, sizeof(size_t) * arrlen);
    }
    
    /// Sets all bits to 1
    inline void fill() {
      for (size_t i = 0;i < arrlen; ++i) array[i] = -1;
      fix_trailing_bits();
    }

    inline bool empty() const {
      for (size_t i = 0; i < arrlen; ++i) if (array[i]) return false;
      return true;
    }
    
    /// Prefetches the word containing the bit b
    inline void prefetch(size_t b) const{
      __builtin_prefetch(&(array[b / (8 * sizeof(size_t))]));
    }
    
    /// Returns the value of the bit b
    inline bool get(size_t b) const{
      size_t arrpos, bitpos;
      bit_to_pos(b, arrpos, bitpos);
      return array[arrpos] & (size_t(1) << size_t(bitpos));
    }
    inline bool operator[](size_t b) const { return get(b); }

    //! Atomically sets the bit at b to true returning the old value
    inline bool set_bit(size_t b) {
      // use CAS to set the bit
      size_t arrpos, bitpos;
      bit_to_pos(b, arrpos, bitpos);
      const size_t mask(size_t(1) << size_t(bitpos)); 
      return __sync_fetch_and_or(array + arrpos, mask) & mask;
    }


    //! Returns the value of the word containing the bit b 
    inline size_t containing_word(size_t b) {
      size_t arrpos, bitpos;
      bit_to_pos(b, arrpos, bitpos);
      return array[arrpos];
    }


    /** Set the bit at position b to true returning the old value.
        Unlike set_bit(), this uses a non-atomic set which is faster,
        but is unsafe if accessed by multiple threads.
    */
    inline bool set_bit_unsync(size_t b) {
      // use CAS to set the bit
      size_t arrpos, bitpos;
      bit_to_pos(b, arrpos, bitpos);
      const size_t mask(size_t(1) << size_t(bitpos)); 
      bool ret = array[arrpos] & mask;
      array[arrpos] |= mask;
      return ret;
    }

    /** Set the state of the bit returning the old value.
      This version uses a non-atomic set which is faster, but
      is unsafe if accessed by multiple threads.
    */
    inline bool set(size_t b, bool value) {
      if (value) return set_bit(b);
      else return clear_bit(b);
    }

    /** Set the state of the bit returning the old value.
      This version uses a non-atomic set which is faster, but
      is unsafe if accessed by multiple threads.
    */
    inline bool set_unsync(size_t b, bool value) {
      if (value) return set_bit_unsync(b);
      else return clear_bit_unsync(b);
    }


    //! Atomically set the bit at b to false returning the old value
    inline bool clear_bit(size_t b) {
      // use CAS to set the bit
      size_t arrpos, bitpos;
      bit_to_pos(b, arrpos, bitpos);
      const size_t test_mask(size_t(1) << size_t(bitpos)); 
      const size_t clear_mask(~test_mask); 
      return __sync_fetch_and_and(array + arrpos, clear_mask) & test_mask;
    }

    /** Clears the state of the bit returning the old value.
      This version uses a non-atomic set which is faster, but
      is unsafe if accessed by multiple threads.
    */
    inline bool clear_bit_unsync(size_t b) {
      // use CAS to set the bit
      size_t arrpos, bitpos;
      bit_to_pos(b, arrpos, bitpos);
      const size_t test_mask(size_t(1) << size_t(bitpos)); 
      const size_t clear_mask(~test_mask); 
      bool ret = array[arrpos] & test_mask;
      array[arrpos] &= clear_mask;
      return ret;
    }


    struct bit_pos_iterator {
      typedef std::input_iterator_tag iterator_category;
      typedef size_t value_type;
      typedef size_t difference_type;
      typedef const size_t reference;
      typedef const size_t* pointer;
      size_t pos;
      const fixed_dense_bitset* db;
      bit_pos_iterator():pos(-1),db(NULL) {}
      bit_pos_iterator(const fixed_dense_bitset* const db, size_t pos):pos(pos),db(db) {}
      
      size_t operator*() const {
        return pos;
      }
      size_t operator++(){
        if (db->next_bit(pos) == false) pos = (size_t)(-1);
        return pos;
      }
      size_t operator++(int){
        size_t prevpos = pos;
        if (db->next_bit(pos) == false) pos = (size_t)(-1);
        return prevpos;
      }
      bool operator==(const bit_pos_iterator& other) const {
        CHECK(db == other.db);
        return other.pos == pos;
      }
      bool operator!=(const bit_pos_iterator& other) const {
        CHECK(db == other.db);
        return other.pos != pos;
      }
    };
    
    typedef bit_pos_iterator iterator;
    typedef bit_pos_iterator const_iterator;

    
    bit_pos_iterator begin() const {
      size_t pos;
      if (first_bit(pos) == false) pos = size_t(-1);
      return bit_pos_iterator(this, pos);
    }
    
    bit_pos_iterator end() const {
      return bit_pos_iterator(this, (size_t)(-1));
    }

    /** Returns true with b containing the position of the 
        first bit set to true.
        If such a bit does not exist, this function returns false.
    */
    inline bool first_bit(size_t &b) const {
      for (size_t i = 0; i < arrlen; ++i) {
        if (array[i]) {
          b = (size_t)(i * (sizeof(size_t) * 8)) + first_bit_in_block(array[i]);
          return true;
        }
      }
      return false;
    }

    /** Returns true with b containing the position of the 
        first bit set to false.
        If such a bit does not exist, this function returns false.
    */
    inline bool first_zero_bit(size_t &b) const {
      for (size_t i = 0; i < arrlen; ++i) {
        if (~array[i]) {
          b = (size_t)(i * (sizeof(size_t) * 8)) + first_bit_in_block(~array[i]);
          return true;
        }
      }
      return false;
    }



    /** Where b is a bit index, this function will return in b,
        the position of the next bit set to true, and return true.
        If all bits after b are false, this function returns false.
    */
    inline bool next_bit(size_t &b) const {
      size_t arrpos, bitpos;
      bit_to_pos(b, arrpos, bitpos);
      //try to find the next bit in this block
      bitpos = next_bit_in_block(bitpos, array[arrpos]);
      if (bitpos != 0) {
        b = (size_t)(arrpos * (sizeof(size_t) * 8)) + bitpos;
        return true;
      }
      else {
        // we have to loop through the rest of the array
        for (size_t i = arrpos + 1; i < arrlen; ++i) {
          if (array[i]) {
            b = (size_t)(i * (sizeof(size_t) * 8)) + first_bit_in_block(array[i]);
            return true;
          }
        }
      }
      return false;
    }
    
    ///  Returns the number of bits in this bitset
    inline size_t size() const {
      return len;
    }
    
    // /// Serializes this bitset to an archive
    // inline void save(oarchive& oarc) const {
    //   //oarc <<len << arrlen;
    //   //if (arrlen > 0)
    //   serialize(oarc, array, arrlen*sizeof(size_t));
    // }
    // 
    // /// Deserializes this bitset from an archive
    // inline void load(iarchive& iarc) {
    //   /*size_t l;
    //   size_t arl;
    //   iarc >> l >> arl;
    //   CHECK_EQ(l, len);
    //   CHECK_EQ(arl, arrlen);*/
    //   //if (arrlen > 0) {
    //   deserialize(iarc, array, arrlen*sizeof(size_t));
    //   //}
    // }

    size_t popcount() const {
      size_t ret = 0;
      for (size_t i = 0;i < arrlen; ++i) {
        ret +=  __builtin_popcountl(array[i]);
      }
      return ret;
    }

    fixed_dense_bitset operator&(const fixed_dense_bitset& other) const {
      CHECK_EQ(size(), other.size());
      fixed_dense_bitset ret(size());
      for (size_t i = 0; i < arrlen; ++i) {
        ret.array[i] = array[i] & other.array[i];
      }
      return ret;
    }


    fixed_dense_bitset operator|(const fixed_dense_bitset& other) const {
      CHECK_EQ(size(), other.size());
      fixed_dense_bitset ret(size());
      for (size_t i = 0; i < arrlen; ++i) {
        ret.array[i] = array[i] | other.array[i];
      }
      return ret;
    }

    fixed_dense_bitset operator-(const fixed_dense_bitset& other) const {
      CHECK_EQ(size(), other.size());
      fixed_dense_bitset ret(size());
      for (size_t i = 0; i < arrlen; ++i) {
        ret.array[i] = array[i] - (array[i] & other.array[i]);
      }
      return ret;
    }


    fixed_dense_bitset& operator&=(const fixed_dense_bitset& other) {
      CHECK_EQ(size(), other.size());
      for (size_t i = 0; i < arrlen; ++i) {
        array[i] &= other.array[i];
      }
      return *this;
    }


    fixed_dense_bitset& operator|=(const fixed_dense_bitset& other) {
      CHECK_EQ(size(), other.size());
      for (size_t i = 0; i < arrlen; ++i) {
        array[i] |= other.array[i];
      }
      return *this;
    }

    fixed_dense_bitset& operator-=(const fixed_dense_bitset& other) {
      CHECK_EQ(size(), other.size());
      for (size_t i = 0; i < arrlen; ++i) {
        array[i] = array[i] - (array[i] & other.array[i]);
      }
      return *this;
    }

    bool operator==(const fixed_dense_bitset& other) const {
      CHECK_EQ(size(), other.size());
      CHECK_EQ(arrlen, other.arrlen);
      bool ret = true;
      for (size_t i = 0; i < arrlen; ++i) {
        ret &= (array[i] == other.array[i]);
      }
      return ret;
    }


  private:
    inline static void bit_to_pos(size_t b, size_t &arrpos, size_t &bitpos) {
      // the compiler better optimize this...
      arrpos = b / (8 * sizeof(size_t));
      bitpos = b & (8 * sizeof(size_t) - 1);
    }
  

    // returns 0 on failure
    inline size_t next_bit_in_block(const size_t &b, const size_t &block) const {
      size_t belowselectedbit = size_t(-1) - (((size_t(1) << b) - 1)|(size_t(1)<<b));
      size_t x = block & belowselectedbit ;
      if (x == 0) return 0;
      else return (size_t)__builtin_ctzl(x);
    }

    // returns 0 on failure
    inline size_t first_bit_in_block(const size_t &block) const {
      // use CAS to set the bit
      if (block == 0) return 0;
      else return (size_t)__builtin_ctzl(block);
    }

    // clears the trailing bits in the last block which are not part
    // of the actual length of the bitset
    void fix_trailing_bits() {
      // how many bits are in the last block
      size_t lastbits = len % (8 * sizeof(size_t));
      if (lastbits == 0) return;
      array[arrlen - 1] &= ((size_t(1) << lastbits) - 1);
    }

 
    static const size_t arrlen;
    size_t array[len / (sizeof(size_t) * 8) + (len % (sizeof(size_t) * 8) > 0)];
  };

  template<int len>
  const size_t fixed_dense_bitset<len>::arrlen = len / (sizeof(size_t) * 8) + (len % (sizeof(size_t) * 8) > 0);
}
