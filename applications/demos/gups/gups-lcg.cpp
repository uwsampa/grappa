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

#include <Grappa.hpp>

#include <Collective.hpp>
#include <ParallelLoop.hpp>
#include <Delegate.hpp>
#include <AsyncDelegate.hpp>
#include <GlobalAllocator.hpp>
#include <Array.hpp>

using namespace Grappa;

DEFINE_int64( sizeA, 1 << 30, "Size of array that GUPS increments" );
DEFINE_int64( sizeB, 1 << 20, "Number of iterations" );

GRAPPA_DEFINE_METRIC( SimpleMetric<double>, gups_runtime, 0.0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<double>, gups_throughput, 0.0 );

const uint64_t lcgM = 6364136223846793005UL;
const uint64_t lcgB = 1442695040888963407UL;
const uint64_t LARGE_PRIME = 18446744073709551557UL;

int main( int argc, char * argv[] ) {

  init( &argc, &argv );
  
  run( [] {

      auto A = global_alloc<int64_t>(FLAGS_sizeA);
      Grappa::memset( A, 0, FLAGS_sizeA );

      double start = walltime();

      forall<unbound>(0, FLAGS_sizeB, [=](int64_t i){
          uint64_t b = (lcgM * i + lcgB) % FLAGS_sizeA;
          delegate::increment<async>( A + b, 1);
        });

      gups_runtime = walltime() - start;
      gups_throughput = FLAGS_sizeB / gups_runtime;

      LOG(INFO) << gups_throughput.value() << " UPS in " << gups_runtime.value() << " seconds";

      global_free(A);

    } );

  finalize();

  return 0;
}
