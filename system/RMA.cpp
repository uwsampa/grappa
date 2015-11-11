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

#include <RMA.hpp>

namespace Grappa {
namespace impl {

RMA global_rma;

int64_t RMA::atomic_op_int64_helper( const Core core, int64_t * dest, const int64_t * source, MPI_Op op ) {
  size_t size = sizeof(int64_t);
  auto dest_int = reinterpret_cast<intptr_t>(dest);
  auto it = get_enclosing( dest_int );
  DVLOG(2) << "Found " << it->second << " for dest " << dest << " size " << size;
  auto base_int = reinterpret_cast<intptr_t>( it->second.base_ );
  auto offset = dest_int - base_int;
  CHECK_LE( offset + size, it->second.size_ ) << "Operation would overrun RMA window";
  
  // TODO: deal with >31-bit offsets and sizes
  CHECK_LT( offset, std::numeric_limits<int>::max() ) << "Operation would overflow MPI argument type";
  CHECK_LT( size,   std::numeric_limits<int>::max() ) << "Operation would overflow MPI argument type";
  int64_t result;
  MPI_CHECK( MPI_Fetch_and_op( source, &result, MPI_INT64_T,
                               core, offset,
                               op,
                               it->second.window_ ) );
  return result;
}

template<>
int64_t RMA::atomic_op< int64_t, op::plus<int64_t> >( const Core core, int64_t * dest, const int64_t * source ) {
  return atomic_op_int64_helper( core, dest, source, MPI_SUM );
}

template<>
int64_t RMA::atomic_op< int64_t, op::bitwise_and<int64_t> >( const Core core, int64_t * dest, const int64_t * source ) {
  return atomic_op_int64_helper( core, dest, source, MPI_BAND );
}

template<>
int64_t RMA::atomic_op< int64_t, op::bitwise_or<int64_t> >( const Core core, int64_t * dest, const int64_t * source ) {
  return atomic_op_int64_helper( core, dest, source, MPI_BOR );
}

template<>
int64_t RMA::atomic_op< int64_t, op::bitwise_xor<int64_t> >( const Core core, int64_t * dest, const int64_t * source ) {
  return atomic_op_int64_helper( core, dest, source, MPI_BXOR );
}

template<>
int64_t RMA::atomic_op< int64_t, op::replace<int64_t> >( const Core core, int64_t * dest, const int64_t * source ) {
  return atomic_op_int64_helper( core, dest, source, MPI_REPLACE );
}


} // namespace impl
} // namespace Grappa

