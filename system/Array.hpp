
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.
#include "Addressing.hpp"
#include "Communicator.hpp"
#include "Collective.hpp"
#include "Cache.hpp"
#include "GlobalCompletionEvent.hpp"
#include "ParallelLoop.hpp"

namespace Grappa {

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

namespace impl {
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
      DVLOG(3) << "nfirstcore = " << nfirstcore;
      
      Writeback w(dst, nfirstcore, src.pointer());
      src_start += nfirstcore;
    }
    if ((src_end-1).core() == mycore()) {
      int64_t nlastcore = src_end - src_end.block_min();
      int64_t index = nelem - nlastcore;
      DVLOG(3) << "nlastcore = " << nlastcore << ", index = " << index;
      CHECK((src+index).core() == mycore());
      Writeback w(dst+index, nlastcore, (src+index).pointer());
      src_end -= nlastcore;
    }
    
    auto * local_base = src_start.localize();
    size_t nlocal_trimmed = src_end.localize() - local_base;
    CHECK_EQ((nlocal_trimmed) % nblock, 0);
    size_t nlocalblocks = nlocal_trimmed/nblock;
    Writeback * ws = locale_alloc<Writeback>(nlocalblocks);
    for (size_t i=0; i<nlocal_trimmed/nblock; i++) {
      size_t j = make_linear(local_base+(i*nblock))-src;
      new (ws+i) Writeback(dst+j, nblock, local_base+(i*nblock));
      ws[i].start_release();
    }
    for (size_t i=0; i<nlocalblocks; i++) { ws[i].block_until_released(); }
    locale_free(ws);
    
    // const size_t nlocal = local_end - local_base;
    // const size_t nlocalblocks = nlocal/nblock + ((nlocal%nblock == 0) ? 0 : 1);
    // Writeback ** putters = new Writeback*[nlocalblocks];
    // for (size_t i=0; i < nlocalblocks; i++) {
    //   size_t j = make_linear(local_base+(i*nblock))-src;
    //   size_t n = (i < nlocalblocks-1) ? nblock : (local_end-local_base)-(i*nblock);
    // 
    //   // initialize WO cache to read from this block locally and write to corresponding block in dest
    //   putters[i] = new Writeback(dst+j, n, local_base+(i*nblock));
    //   putters[i]->start_release();
    // }
    // for (size_t i=0; i < nlocalblocks; i++) { delete putters[i]; }
    // delete [] putters;
  }
}

/// Memcpy over Grappa global arrays. Arguments `dst` and `src` must point into global arrays 
/// (so must be linear addresses) and be non-overlapping, and both must have at least `nelem`
/// elements.
template< typename T >
void memcpy(GlobalAddress<T> dst, GlobalAddress<T> src, size_t nelem) {
  on_all_cores([dst,src,nelem]{
    impl::do_memcpy_locally(dst,src,nelem);
  });
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
}

namespace util {

  template< int VERBOSITY = 1, typename T = decltype(nullptr)>
  inline void print_array(const char * name, T * base, size_t nelem, int width = 10) {
    std::stringstream ss; ss << "\n" << name << ": [";
    for (size_t i=0; i<nelem; i++) {
      if (i % width == 0) ss << "\n  ";
      ss << " " << base[i];
    }
    ss << "\n]"; VLOG(VERBOSITY) << ss.str();
  }
  
  template< int VERBOSITY = 1, typename T = decltype(nullptr) >
  inline void print_array(const char * name, GlobalAddress<T> base, size_t nelem, int width = 10) {
    std::stringstream ss; ss << "\n" << name << ": [";
    for (size_t i=0; i<nelem; i++) {
      if (i % width == 0) ss << "\n  ";
      ss << " " << delegate::read(base+i);
    }
    ss << "\n]"; VLOG(VERBOSITY) << ss.str();
  }
  
}

} // namespace Grappa

/// Legacy: @see { Grappa::memset() }
template< typename T , typename S >
void Grappa_memset(GlobalAddress<T> request_address, S value, size_t count) {
  Grappa::memset(request_address, value, count);
}

/// Legacy: @see { Grappa::memset() }
template< typename T, typename S >
void Grappa_memset_local(GlobalAddress<T> base, S value, size_t count) {
  Grappa::memset(base, value, count);
}

template< typename T >
void Grappa_memcpy(GlobalAddress<T> dst, GlobalAddress<T> src, size_t nelem) {
  Grappa::memcpy(dst,src,nelem);
}
