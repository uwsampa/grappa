#pragma once
////////////////////////////////////////////////////////////////////////
// This file is part of Grappa, a system for scaling irregular
// applications on commodity clusters. 

// Copyright (C) 2010-2014 University of Washington and Battelle
// Memorial Institute. University of Washington authorizes use of this
// Grappa software.

// Grappa is free software: you can redistribute it and/or modify it
// under the terms of the Affero General Public License as published
// by Affero, Inc., either version 1 of the License, or (at your
// option) any later version.

// Grappa is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// Affero General Public License for more details.

// You should have received a copy of the Affero General Public
// License along with this program. If not, you may obtain one from
// http://www.affero.org/oagpl.html.
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

char * deaggregate_amessage_buffer( char * buf, size_t size );


template< typename T >
std::ostream& operator<<( std::ostream& o, const NTMessage<T>& m ) {
  const NTMessageBase * mb = &m;
  return o << *mb;
}


} // namespace impl
} // namespace Grappa
