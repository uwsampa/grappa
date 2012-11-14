
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.
#include <Grappa.hpp>
#include <Addressing.hpp>
#include <ForkJoin.hpp>

struct ConstReplyArgs {
  int64_t replies_left;
  Thread * sleeper;
};

template< typename T >
struct ConstRequestArgs {
  GlobalAddress<T> addr;
  size_t count;
  T value;
  GlobalAddress<ConstReplyArgs> reply;
};

static void memset_reply_am(GlobalAddress<ConstReplyArgs> * reply, size_t sz, void * payload, size_t psz) {
  CHECK(reply->node() == Grappa_mynode());
  ConstReplyArgs * r = reply->pointer();
  (r->replies_left)--;
  if (r->replies_left == 0) {
    Grappa_wake(r->sleeper);
  }
}

template< typename T >
static void memset_request_am(ConstRequestArgs<T> * args, size_t sz, void* payload, size_t psz) {
  CHECK(args->addr.node() == Grappa_mynode()) << "args->addr.node() = " << args->addr.node();
  T * ptr = args->addr.pointer();
  for (size_t i=0; i<args->count; i++) {
    ptr[i] = args->value;
  }
  Grappa_call_on(args->reply.node(), &memset_reply_am, &args->reply);
}

/// Initialize an array of elements of generic type with a given value.
/// 
/// This version sends a large number of active messages, the same way as the Incoherent
/// releaser, to set each part of a global array. In theory, this version should be able
/// to be called from multiple locations at the same time (to initialize different regions of global memory).
/// 
/// @param base Base address of the array to be set.
/// @param value Value to set every element of array to (will be copied to all the nodes)
/// @param count Number of elements to set, starting at the base address.
template< typename T , typename S >
static void Grappa_memset(GlobalAddress<T> request_address, S value, size_t count) {
  size_t offset = 0;
  size_t request_bytes = 0;
  
  ConstReplyArgs reply;
  reply.replies_left = 0;
  reply.sleeper = CURRENT_THREAD;
  
  ConstRequestArgs<T> args;
  args.addr = request_address;
  args.value = (T)value;
  args.reply = make_global(&reply);
  
  for (size_t total_bytes = count*sizeof(T); offset < total_bytes; offset += request_bytes) {
    // compute number of bytes remaining in the block containing args.addr
    request_bytes = (args.addr.first_byte().block_max() - args.addr.first_byte());
    if (request_bytes > total_bytes - offset) {
      request_bytes = total_bytes - offset;
    }
    CHECK(request_bytes % sizeof(T) == 0);
    args.count = request_bytes / sizeof(T);
    
    reply.replies_left++;
    Grappa_call_on(args.addr.node(), &memset_request_am, &args);
    
    args.addr += args.count;
  }
  
  while (reply.replies_left > 0) Grappa_suspend();
}



LOOP_FUNCTOR_TEMPLATED(T, memset_func, nid, ((GlobalAddress<T>,base)) ((T,value)) ((size_t,count))) {
  T * local_base = base.localize(), * local_end = (base+count).localize();
  for (size_t i=0; i<local_end-local_base; i++) {
    local_base[i] = value;
  }
}

/// Does memset across a global array using a single task on each node and doing local assignments
/// Uses 'GlobalAddress::localize()' to determine the range of actual memory from the global array
/// on a particular node.
/// 
/// Must be called by itself (preferably from the user_main task) because it contains a call to
/// fork_join_custom().
///
/// @see Grappa_memset()
///
/// @param base Base address of the array to be set.
/// @param value Value to set every element of array to (will be copied to all the nodes)
/// @param count Number of elements to set, starting at the base address.
template< typename T, typename S >
void Grappa_memset_local(GlobalAddress<T> base, S value, size_t count) {
  {
    memset_func<T> f(base, (T)value, count);
    fork_join_custom(&f);
  }
}

LOOP_FUNCTOR_TEMPLATED(T, memcpy_func, nid, ((GlobalAddress<T>,dst)) ((GlobalAddress<T>,src)) ((size_t,nelem))) {
  typedef typename Incoherent<T>::WO Writeback;

  T * local_base = src.localize(), * local_end = (src+nelem).localize();
  const size_t nblock = block_size / sizeof(T);
  const size_t nlocalblocks = (local_end-local_base)/nblock;
  Writeback ** putters = new Writeback*[nlocalblocks];
  for (size_t i=0; i < nlocalblocks; i++) {
    size_t j = make_linear(local_base+(i*nblock))-src;
    size_t n = (i < nlocalblocks-1) ? nblock : (local_end-local_base)-(i*nblock);

    // initialize WO cache to read from this block locally and write to corresponding block in dest
    putters[i] = new Writeback(dst+j, n, local_base+(i*nblock));
    putters[i]->start_release();
  }
  for (size_t i=0; i < nlocalblocks; i++) { delete putters[i]; }
  delete [] putters;
}

template< typename T >
void Grappa_memcpy(GlobalAddress<T> dst, GlobalAddress<T> src, size_t nelem) {
  memcpy_func<T> f(dst,src,nelem);
  fork_join_custom(&f);
}
