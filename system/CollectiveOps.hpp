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

namespace Grappa {

/// Operations used for raw communicator atomic operations and collectives.
/// (compared to MPI, just missing maxloc, minloc)
/// TODO: reconcile with Collective.hpp
namespace op {

template< typename T > struct max         { T operator()( const T & lhs, const T & rhs ) const { return std::max(lhs, rhs); } };
template< typename T > struct min         { T operator()( const T & lhs, const T & rhs ) const { return std::min(lhs, rhs); } };

template< typename T > struct plus        { T operator()( const T & lhs, const T & rhs ) const { return  lhs +   rhs;       } };
template< typename T > struct sum         { T operator()( const T & lhs, const T & rhs ) const { return  lhs +   rhs;       } };

template< typename T > struct prod        { T operator()( const T & lhs, const T & rhs ) const { return  lhs *   rhs;       } };

template< typename T > struct bitwise_and { T operator()( const T & lhs, const T & rhs ) const { return  lhs &   rhs;       } };
template< typename T > struct bitwise_or  { T operator()( const T & lhs, const T & rhs ) const { return  lhs |   rhs;       } };
template< typename T > struct bitwise_xor { T operator()( const T & lhs, const T & rhs ) const { return  lhs ^   rhs;       } };

template< typename T > struct logical_and { T operator()( const T & lhs, const T & rhs ) const { return  lhs &&  rhs;       } };
template< typename T > struct logical_or  { T operator()( const T & lhs, const T & rhs ) const { return  lhs ||  rhs;       } };
template< typename T > struct logical_xor { T operator()( const T & lhs, const T & rhs ) const { return !lhs != !rhs;       } };

template< typename T > struct replace     { T operator()( const T & lhs, const T & rhs ) const { return          rhs;       } };

template< typename T > struct no_op       { T operator()( const T & lhs, const T & rhs ) const { return  lhs        ;       } };

// these need to be fixed to handle lhs.first == rhs.first properly
//template< typename T > struct maxloc      { T operator()( const T & lhs, const T & rhs ) const { return lhs.first > rhs.first ? lhs : rhs; } };
//template< typename T > struct minloc      { T operator()( const T & lhs, const T & rhs ) const { return lhs.first < rhs.first ? lhs : rhs; } };
}

} // namespace Grappa
