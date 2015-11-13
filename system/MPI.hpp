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

#include <mpi.h>
#include <functional>
#include <complex>

#include "mpi.h"

#include "CollectiveOps.hpp"


#define MPI_CHECK( mpi_call )                                           \
  do {                                                                  \
    int retval;                                                         \
    if( (retval = (mpi_call)) != 0 ) {                                  \
      char error_string[MPI_MAX_ERROR_STRING];                          \
      int length;                                                       \
      MPI_Error_string( retval, error_string, &length);                 \
      LOG(FATAL) << "MPI call failed: " #mpi_call ": "                  \
                 << error_string;                                       \
    }                                                                   \
  } while(0)

namespace Grappa {
namespace impl {

///
/// Equivalences between C++ and MPI types.
/// Some are commented out since the type is an alias for another type / MPI datatype; the 
///
template< typename T > struct MPIDatatype;
template<> struct MPIDatatype< char                      > { static const MPI_Datatype value = MPI_CHAR; };
template<> struct MPIDatatype< signed short int          > { static const MPI_Datatype value = MPI_SHORT; };
template<> struct MPIDatatype< signed int                > { static const MPI_Datatype value = MPI_INT; };
template<> struct MPIDatatype< signed long int           > { static const MPI_Datatype value = MPI_LONG; };
template<> struct MPIDatatype< signed long long int      > { static const MPI_Datatype value = MPI_LONG_LONG_INT; };
// template<> struct MPIDatatype< signed long long          > { static const MPI_Datatype value = MPI_LONG_LONG; };
template<> struct MPIDatatype< signed char               > { static const MPI_Datatype value = MPI_SIGNED_CHAR; };
template<> struct MPIDatatype< unsigned char             > { static const MPI_Datatype value = MPI_UNSIGNED_CHAR; };
template<> struct MPIDatatype< unsigned short int        > { static const MPI_Datatype value = MPI_UNSIGNED_SHORT; };
template<> struct MPIDatatype< unsigned int              > { static const MPI_Datatype value = MPI_UNSIGNED; };
template<> struct MPIDatatype< unsigned long int         > { static const MPI_Datatype value = MPI_UNSIGNED_LONG; };
template<> struct MPIDatatype< unsigned long long int    > { static const MPI_Datatype value = MPI_UNSIGNED_LONG_LONG; };
template<> struct MPIDatatype< float                     > { static const MPI_Datatype value = MPI_FLOAT; };
template<> struct MPIDatatype< double                    > { static const MPI_Datatype value = MPI_DOUBLE; };
template<> struct MPIDatatype< long double               > { static const MPI_Datatype value = MPI_LONG_DOUBLE; };
template<> struct MPIDatatype< wchar_t                   > { static const MPI_Datatype value = MPI_WCHAR; };
// template<> struct MPIDatatype< _Bool                     > { static const MPI_Datatype value = MPI_C_BOOL; };
// template<> struct MPIDatatype< int8_t                    > { static const MPI_Datatype value = MPI_INT8_T; };
// template<> struct MPIDatatype< int16_t                   > { static const MPI_Datatype value = MPI_INT16_T; };
// template<> struct MPIDatatype< int32_t                   > { static const MPI_Datatype value = MPI_INT32_T; };
// template<> struct MPIDatatype< int64_t                   > { static const MPI_Datatype value = MPI_INT64_T; };
// template<> struct MPIDatatype< uint8_t                   > { static const MPI_Datatype value = MPI_UINT8_T; };
// template<> struct MPIDatatype< uint16_t                  > { static const MPI_Datatype value = MPI_UINT16_T; };
// template<> struct MPIDatatype< uint32_t                  > { static const MPI_Datatype value = MPI_UINT32_T; };
// template<> struct MPIDatatype< uint64_t                  > { static const MPI_Datatype value = MPI_UINT64_T; };
// template<> struct MPIDatatype< float _Complex            > { static const MPI_Datatype value = MPI_C_COMPLEX; };
template<> struct MPIDatatype< float _Complex            > { static const MPI_Datatype value = MPI_C_FLOAT_COMPLEX; };
template<> struct MPIDatatype< double _Complex           > { static const MPI_Datatype value = MPI_C_DOUBLE_COMPLEX; };
template<> struct MPIDatatype< long double _Complex      > { static const MPI_Datatype value = MPI_C_LONG_DOUBLE_COMPLEX; };
// template<> struct MPIDatatype< unsigned char             > { static const MPI_Datatype value = MPI_BYTE; };
// template<> struct MPIDatatype< MPI_Aint                  > { static const MPI_Datatype value = MPI_AINT; };
// template<> struct MPIDatatype< MPI_Offset                > { static const MPI_Datatype value = MPI_OFFSET; };
// template<> struct MPIDatatype< MPI_Count                 > { static const MPI_Datatype value = MPI_COUNT; };
template<> struct MPIDatatype< bool                      > { static const MPI_Datatype value = MPI_CXX_BOOL; };
template<> struct MPIDatatype< std::complex<float>       > { static const MPI_Datatype value = MPI_CXX_FLOAT_COMPLEX; };
template<> struct MPIDatatype< std::complex<double>      > { static const MPI_Datatype value = MPI_CXX_DOUBLE_COMPLEX; };
template<> struct MPIDatatype< std::complex<long double> > { static const MPI_Datatype value = MPI_CXX_LONG_DOUBLE_COMPLEX; };

///
/// Equivalences between certain known C++ functors and MPI operation types for collective/atomic ops
///
template< typename T, typename OP > struct MPIOp;
template< typename T > struct MPIOp< T, std::plus<T>       > { static const MPI_Op value = MPI_SUM; };
template< typename T > struct MPIOp< T, std::less<T>       > { static const MPI_Op value = MPI_MIN; };
template< typename T > struct MPIOp< T, std::greater<T>    > { static const MPI_Op value = MPI_MAX; };
template< typename T > struct MPIOp< T, op::max<T>         > { static const MPI_Op value = MPI_MAX; };
template< typename T > struct MPIOp< T, op::min<T>         > { static const MPI_Op value = MPI_MIN; };
template< typename T > struct MPIOp< T, op::plus<T>        > { static const MPI_Op value = MPI_SUM; };
template< typename T > struct MPIOp< T, op::sum<T>         > { static const MPI_Op value = MPI_SUM; };
template< typename T > struct MPIOp< T, op::prod<T>        > { static const MPI_Op value = MPI_PROD; };
template< typename T > struct MPIOp< T, op::bitwise_and<T> > { static const MPI_Op value = MPI_BAND; };
template< typename T > struct MPIOp< T, op::bitwise_or<T>  > { static const MPI_Op value = MPI_BOR; };
template< typename T > struct MPIOp< T, op::bitwise_xor<T> > { static const MPI_Op value = MPI_BXOR; };
template< typename T > struct MPIOp< T, op::logical_and<T> > { static const MPI_Op value = MPI_LAND; };
template< typename T > struct MPIOp< T, op::logical_or<T>  > { static const MPI_Op value = MPI_LOR; };
template< typename T > struct MPIOp< T, op::logical_xor<T> > { static const MPI_Op value = MPI_LXOR; };
template< typename T > struct MPIOp< T, op::replace<T>     > { static const MPI_Op value = MPI_REPLACE; };
template< typename T > struct MPIOp< T, op::no_op<T>       > { static const MPI_Op value = MPI_NO_OP; };
// template< typename T > struct MPIOp< T, op::maxloc<T>      > { static const MPI_Op value = MPI_MINLOC; };
// template< typename T > struct MPIOp< T, op::minloc<T>      > { static const MPI_Op value = MPI_MAXLOC; };

} // namespace impl
} // namespace Grappa
