////////////////////////////////////////////////////////////////////////
// Copyright (c) 2010-2015, University of Washington and Battelle
// Memorial Institute.  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//     * Redistributions of source code must retain the above
//       copyright notice, this list of conditions and the following
//       disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials
//       provided with the distribution.
//     * Neither the name of the University of Washington, Battelle
//       Memorial Institute, or the names of their contributors may be
//       used to endorse or promote products derived from this
//       software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// UNIVERSITY OF WASHINGTON OR BATTELLE MEMORIAL INSTITUTE BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
////////////////////////////////////////////////////////////////////////
#pragma once

#include "Addressing.hpp"
#include "Communicator.hpp"
#include "Collective.hpp"
#include "Cache.hpp"
#include "GlobalCompletionEvent.hpp"
#include "ParallelLoop.hpp"
#include "GlobalAllocator.hpp"
#include <type_traits>
#include "Delegate.hpp"

namespace Grappa {
/// @addtogroup Containers
/// @{
  
/// Initialize an array of elements of generic type with a given value.
/// 
/// This version sends a large number of active messages, the same way as the Incoherent
/// releaser, to set each part of a global array. In theory, this version should be able
/// to be called from multiple locations at the same time (to initialize different regions of global memory).
/// 
/// @param base Base address of the array to be set.
/// @param value Value to set every element of array to (will be copied to all the nodes)
/// @param count Number of elements to set, starting at the base address.
template< typename T, typename S >
void memset(GlobalAddress<T> base, S value, size_t count) {
  call_on_all_cores([base,count,value]{
    T * local_base = base.localize();
    T * local_end = (base+count).localize();
    for (size_t i=0; i<local_end-local_base; i++) {
      local_base[i] = value;
    }
  });
}

/// Type-based memset for local arrays to match what is provided for distributed arrays.
template< typename T, typename S >
void memset(T* base, S value, size_t count) {
  for (size_t i=0; i < count; i++) {
    base[i] = value;
  }
}

namespace impl {
  /// Copy elements of array (src..src+nelem) that are local to corresponding locations in dst
  template< typename T >
  void do_memcpy_locally(GlobalAddress<T> dst, GlobalAddress<T> src, size_t nelem) {
    typedef typename Incoherent<T>::WO Writeback;
    auto src_end = src+nelem;
    // T * local_base = src.localize(), * local_end = (src+nelem).localize();
    int64_t nfirstcore = src.block_max() - src;
    int64_t nlastcore = src_end - src_end.block_min();
    int64_t nmiddle = nelem - nfirstcore - nlastcore;
    
    const size_t nblock = block_size / sizeof(T);
    
    CHECK_EQ(nmiddle % nblock, 0);

    auto src_start = src;
    if (src.core() == mycore()) {
      int64_t nfirstcore = src.block_max() - src;
      if (nfirstcore > 0 && nlastcore != nblock) {
        DVLOG(3) << "nfirstcore = " << nfirstcore;
        Writeback w(dst, nfirstcore, src.pointer());
        src_start += nfirstcore;
      }
    }
    if ((src_end-1).core() == mycore()) {
      int64_t nlastcore = src_end - src_end.block_min();
      int64_t index = nelem - nlastcore;
      if (nlastcore > 0 && nlastcore != nblock) {
        DVLOG(3) << "nlastcore = " << nlastcore << ", index = " << index;
        CHECK((src+index).core() == mycore());
        Writeback w(dst+index, nlastcore, (src+index).pointer());
        src_end -= nlastcore;
      }
    }
    
    auto * local_base = src_start.localize();
    size_t nlocal_trimmed = src_end.localize() - local_base;
    CHECK_EQ((nlocal_trimmed) % nblock, 0);
    size_t nlocalblocks = nlocal_trimmed/nblock;
    Writeback * ws = locale_alloc<Writeback>(nlocalblocks);
    for (size_t i=0; i<nlocalblocks; i++) {
      size_t j = make_linear(local_base+(i*nblock))-src;
      new (ws+i) Writeback(dst+j, nblock, local_base+(i*nblock));
      ws[i].start_release();
    }
    for (size_t i=0; i<nlocalblocks; i++) { ws[i].block_until_released(); }
    locale_free(ws);
  }
}

/// Memcpy over Grappa global arrays. Arguments `dst` and `src` must point into global arrays 
/// (so must be linear addresses) and be non-overlapping, and both must have at least `nelem`
/// elements.
template< typename T >
void memcpy(GlobalAddress<T> dst, GlobalAddress<T> src, size_t nelem) {
  if (src.is_2D() && dst.is_2D()) {
    typename Incoherent<T>::RO c(src, nelem, dst.pointer());
    c.block_until_acquired();
  } else {
    on_all_cores([dst,src,nelem]{
      impl::do_memcpy_locally(dst,src,nelem);
    });
  }
}

/// Helper so we don't have to change the code if we change a Global pointer to a normal pointer (in theory).
template< typename T >
inline void memcpy(T* dst, T* src, size_t nelem) {
  ::memcpy(dst, src, nelem*sizeof(T));
}

template<>
inline void memcpy<void>(void* dst, void* src, size_t nelem) {
  ::memcpy(dst, src, nelem);
}

  

/// Asynchronous version of memcpy, spawns only on cores with array elements. Synchronizes
/// with given GlobalCompletionEvent, so memcpy's are known to be complete after GCE->wait().
/// Note: same restrictions on `dst` and `src` as Grappa::memcpy).
template< GlobalCompletionEvent * GCE = &impl::local_gce, typename T = void >
void memcpy_async(GlobalAddress<T> dst, GlobalAddress<T> src, size_t nelem) {
  on_cores_localized_async<GCE>(src, nelem, [dst,src,nelem](T* base, size_t nlocal){
    impl::do_memcpy_locally(dst,src,nelem);
  });
}

/// not implemented yet
template< typename T >
void prefix_sum(GlobalAddress<T> array, size_t nelem) {
  // not implemented
  CHECK(false) << "prefix_sum is currently unimplemented!";
}

namespace util {
  
  /// String representation of a global array.
  /// 
  /// @code
  ///   GlobalAddress<int> xs;
  ///   DVLOG(2) << array_str("x", xs, 4);
  /// // (if DEBUG=1 and --v=2)
  /// //> x: [
  /// //>  7 4 2 3
  /// //> ]
  /// @endcode
  template<typename T>
  inline std::string array_str(const char * name, GlobalAddress<T> base, size_t nelem, int width = 10) {
    std::stringstream ss; ss << "\n" << name << ": [";
    for (size_t i=0; i<nelem; i++) {
      if (i % width == 0) ss << "\n  ";
      ss << " " << delegate::read(base+i);
    }
    ss << "\n]";
    return ss.str();
  }

  template< int width = 10, typename ArrayT = nullptr_t >
  inline std::string array_str(const char * name, const ArrayT& array) {
    bool multiline = array.size() > width;
    std::stringstream ss;
    if (name) {
      ss << name << ": ";
    }
    ss << "[";
    long i=0;
    for (auto e : array) {
      if (i % width == 0 && multiline) ss << "\n  ";
      ss << " " << e;
      i++;
    }
    ss << (multiline ? "\n" : " ") << "]";
    return ss.str();
  }
  
  template< int width = 10, typename ArrayT = nullptr_t >
  inline std::string array_str(const ArrayT& array) {
    return array_str(nullptr, array);
  }
  
  
  template<typename T>
  struct SimpleIterator {
    T * base;
    size_t nelem;
    T * begin() { return base; }
    T * end()   { return base+nelem; }
    const T * begin() const { return base; }
    const T * end() const { return base+nelem; }
    size_t size() const { return nelem; }
  };
  
  /// Easier C++11 iteration over local array. Similar idea to Addressing::iterate_local().
  ///
  /// @code
  ///   auto array = new long[N];
  ///   for (auto& v : util::iterate(array,N)) {
  ///     v++;
  ///   }
  /// @endcode
  template<typename T>
  SimpleIterator<T> iterate(T* base = nullptr, size_t nelem = 0) { return SimpleIterator<T>{base, nelem}; }
  
  /// String representation of a local array, matches form of Grappa::array_str that takes a global array.
  template< int width = 10, typename T = nullptr_t >
  inline std::string array_str(const char * name, T * base, size_t nelem) {
    return array_str(name, iterate(base, nelem));
  }
  
} // namespace util

/// @}
} // namespace Grappa

template< typename T >
std::ostream& operator<<(std::ostream& o, std::vector<T> v) {
  o << Grappa::util::array_str(nullptr, &v[0], v.size());
  return o;
}
