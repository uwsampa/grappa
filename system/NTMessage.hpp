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

//template< typename T >
struct AMessageBase {
  Core     dest_;
  uint16_t size_;
  uint32_t fp_;
  AMessageBase(Core dest, uint16_t size, uint32_t fp): dest_(dest), size_(size), fp_(fp) { }
}  __attribute__((aligned(8)));

std::ostream& operator<<( std::ostream& o, const AMessageBase& m ) {
  uint64_t fp = m.fp_;
  return o << "<Amessage core:" << m.dest_ << " size:" << m.size_ << " fp:" << (void*) fp << ">";
}

template< typename T >
static inline const uint32_t make_32bit( const T t ) {
  auto tt = reinterpret_cast<void*>( t );
  auto ttt = reinterpret_cast<uint64_t>( tt );
  auto tttt = ttt & 0xffffffffULL;
  CHECK_EQ( ttt, tttt ) << "Function pointer not representable in 32 bits!";
  return tttt;
}

template< typename T >
struct AMessage : AMessageBase {
  T storage_;

  inline AMessage( )
    : AMessageBase()
    , storage_()
  { }

  inline AMessage( Core dest, T t )
    : AMessageBase( dest, sizeof(*this), make_32bit(&deserialize_and_call) )
    , storage_( t )
  { }

  friend char * run_deserialize_and_call( char * c );
public:
    AMessage( const AMessage& m ) = delete; ///< Not allowed.
    AMessage& operator=( const AMessage& m ) = delete;         ///< Not allowed.
    AMessage& operator=( AMessage&& m ) = delete;

    AMessage( AMessage&& m ) = default;

    static char * deserialize_and_call( char * t ) {
      //DVLOG(5) << "In " << __PRETTY_FUNCTION__;
      AMessage<T> * tt = reinterpret_cast< AMessage<T> * >( t );
      tt->storage_();
      return t + sizeof( AMessage<T> );
    }
}  __attribute__((aligned(8)));


typedef char * (*deserializer_t)(char*);


char * deaggregate_amessage_buffer( char * buf, size_t size ) {
  const char * end = buf + size;
  while( buf < end ) {
    auto mb = reinterpret_cast<AMessageBase*>(buf);
    uint64_t fp_int = mb->fp_;
    auto fp = reinterpret_cast<deserializer_t>(fp_int);
    char * next = (*fp)(buf);
    buf = next;
  }
  return buf;
}


template< typename T >
std::ostream& operator<<( std::ostream& o, const AMessage<T>& m ) {
  const AMessageBase * mb = &m;
  return o << *mb;
}


template< typename T >
AMessage<T> send_amessage( Core dest, T t ) {
  AMessage<T> m(dest,t);
  LOG(INFO) << "AMessageBase " << m;
  t();
  return m;
}

} // namespace impl
} // namespace Grappa
