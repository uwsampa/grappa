#pragma once
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

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <type_traits>
#include <functional>
#include <iostream>

typedef int16_t Core;

namespace Grappa {
namespace impl {

struct NTMessageBase {
  Core     dest_;
  uint16_t size_;
  uint32_t fp_;
  NTMessageBase(Core dest, uint16_t size, uint32_t fp): dest_(dest), size_(size), fp_(fp) { }
  NTMessageBase(): dest_(-1), size_(0), fp_(0) { }
  NTMessageBase( const NTMessageBase& m ): dest_(m.dest_), size_(m.size_), fp_(m.fp_) { }
  NTMessageBase( NTMessageBase&& m ): dest_(m.dest_), size_(m.size_), fp_(m.fp_) { }
}  __attribute__((aligned(8)));

std::ostream& operator<<( std::ostream& o, const NTMessageBase& m );

template< typename T >
static inline const uint32_t make_32bit( const T t ) {
  auto tt = reinterpret_cast<void*>( t );
  auto ttt = reinterpret_cast<uint64_t>( tt );
  auto tttt = ttt & 0xffffffffULL;
  CHECK_EQ( ttt, tttt ) << "Function pointer not representable in 32 bits!";
  return tttt;
}

template< typename T >
struct NTMessage : NTMessageBase {
  T storage_;

  inline NTMessage( )
    : NTMessageBase()
    , storage_()
  { }

  inline NTMessage( Core dest, T t )
    : NTMessageBase( dest, sizeof(*this), make_32bit(&deserialize_and_call) )
    , storage_( t )
  { }

  friend char * run_deserialize_and_call( char * c );
public:
    NTMessage( const NTMessage& m ) = delete; ///< Not allowed.
    NTMessage& operator=( const NTMessage& m ) = delete;         ///< Not allowed.
    NTMessage& operator=( NTMessage&& m ) = delete;

    NTMessage( NTMessage&& m ) = default;

    static char * deserialize_and_call( char * t ) {
      //DVLOG(5) << "In " << __PRETTY_FUNCTION__;
      NTMessage<T> * tt = reinterpret_cast< NTMessage<T> * >( t );
      tt->storage_();
      return t + sizeof( NTMessage<T> );
    }
}  __attribute__((aligned(8)));


typedef char * (*deserializer_t)(char*);

char * deaggregate_nt_buffer( char * buf, size_t size );


template< typename T >
std::ostream& operator<<( std::ostream& o, const NTMessage<T>& m ) {
  const NTMessageBase * mb = &m;
  return o << *mb;
}


} // namespace impl
} // namespace Grappa
