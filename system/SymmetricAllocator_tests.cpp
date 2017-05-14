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

#include <boost/test/unit_test.hpp>
#include "Grappa.hpp"
#include "SymmetricAllocator.hpp"

using namespace Grappa;

BOOST_AUTO_TEST_SUITE( SymmetricAllocator_tests );

BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa::init( GRAPPA_TEST_ARGS );

  int * p = Grappa::spmd::blocking::symmetric_alloc<int>(1 << 3);
  BOOST_CHECK_EQUAL( p, reinterpret_cast<int*>( 0x40000000010ULL ) );
  if( 0 == mycore() ) LOG(INFO) << "p: " << p;

  int * q = Grappa::spmd::blocking::symmetric_alloc<int>(1 << 3);
  BOOST_CHECK_EQUAL( q, reinterpret_cast<int*>( 0x40000000040ULL ) );
  if( 0 == mycore() ) LOG(INFO) << "q: " << q;

  Grappa::spmd::blocking::symmetric_free(p);

  int * r = Grappa::spmd::blocking::symmetric_alloc<int>(1 << 3);
  BOOST_CHECK_EQUAL( r, reinterpret_cast<int*>( 0x40000000010ULL ) );
  if( 0 == mycore() ) LOG(INFO) << "r: " << r;
    
  int * s = Grappa::spmd::blocking::symmetric_alloc<int>(1 << 22);
  BOOST_CHECK_EQUAL( s, reinterpret_cast<int*>( 0x40000000070ULL ) );
  if( 0 == mycore() ) LOG(INFO) << "s: " << s;

  Grappa::spmd::blocking::symmetric_free(r);
  
  int * t = Grappa::spmd::blocking::symmetric_alloc<int>(1 << 3);
  BOOST_CHECK_EQUAL( t, reinterpret_cast<int*>( 0x40000000010ULL ) );
  if( 0 == mycore() ) LOG(INFO) << "t: " << t;

  int * u = Grappa::spmd::blocking::symmetric_alloc<int>(1 << 3);
  BOOST_CHECK_EQUAL( u, reinterpret_cast<int*>( 0x40001000080ULL ) );
  if( 0 == mycore() ) LOG(INFO) << "u: " << u;

  int * v = Grappa::spmd::blocking::symmetric_alloc<int>(1 << 3);
  BOOST_CHECK_EQUAL( v, reinterpret_cast<int*>( 0x400010000b0ULL ) );
  if( 0 == mycore() ) LOG(INFO) << "v: " << v;

  Grappa::impl::global_morecore.cleanup();

  Grappa::finalize();
}

BOOST_AUTO_TEST_SUITE_END();
