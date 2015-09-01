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

#include <iostream>
#include <boost/test/unit_test.hpp>
#include "Grappa.hpp"
#include "Metrics.hpp"
#include "Delegate.hpp"
#include "Collective.hpp"
#include "HistogramMetric.hpp"
#include "GlobalAllocator.hpp"
#include "ParallelLoop.hpp"
#include "PerformanceTools.hpp"

BOOST_AUTO_TEST_SUITE( Metrics_tests );

using namespace Grappa;

//equivalent to: static SimpleMetric<int> foo("foo", 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<int>, foo, 0);
#define I_FOO 0

//equivalent to: static SimpleMetric<double> bar("bar", 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, bar, 0);
#define I_BAR 1

GRAPPA_DEFINE_METRIC(SummarizingMetric<int>, baz, 0);
#define I_BAZ 2

GRAPPA_DEFINE_METRIC(HistogramMetric, rand_msg, 0);
#define I_HIST 3

GRAPPA_DEFINE_METRIC(MaxMetric<uint64_t>, maz, 0);
#define I_MAZ 4

GRAPPA_DEFINE_METRIC(StringMetric, foostr, "");
#define I_FOOSTR 5

BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa::init( GRAPPA_TEST_ARGS );
  Grappa::run([]{
    CHECK(Grappa::cores() >= 2); // at least 2 nodes for these tests...

    foo++;
    bar = 3.14;

    baz += 1;
    baz += 4;
    baz += 9;

    foostr = "foo";

    delegate::call(1, []() -> bool {
      foo++;
      foo++;
      bar = 5.41;
      baz += 16;
      baz += 25;
      baz += 36;
      foostr = "bar";
      foostr += "z";

      BOOST_CHECK( baz.value() == (16+25+36) );
      BOOST_CHECK( foo.value() == 2 );
      BOOST_CHECK( bar.value() == 5.41 );
      BOOST_MESSAGE( "foostr => " << foostr.value() );
      BOOST_CHECK( foostr.value() == "barz" );

      return true;
    });

    BOOST_MESSAGE( "foostr => " << foostr.value() );
    BOOST_CHECK( foostr.value() == "foo" );
  
    Metrics::print();

    delegate::call(1, []() -> bool {
      Metrics::print();
      return true;
    });
   
    // test merge 
    std::vector<impl::MetricBase*> all;
    Metrics::merge(all);
    BOOST_CHECK( reinterpret_cast<SimpleMetric<int>*>(all[I_FOO])->value() == 3 );
    BOOST_CHECK( reinterpret_cast<SimpleMetric<double>*>(all[I_BAR])->value() == 8.55 );
    BOOST_CHECK( reinterpret_cast<SummarizingMetric<int>*>(all[I_BAZ])->value() == 91 );
    // string append not associative
    BOOST_CHECK( (reinterpret_cast<StringMetric*>(all[I_FOOSTR])->value() == "foobarz") 
                || (reinterpret_cast<StringMetric*>(all[I_FOOSTR])->value() == "barzfoo") );
      

    Metrics::reset_all_cores();
    Metrics::start_tracing();

#ifdef HISTOGRAM_SAMPLED
    VLOG(1) << "testing histogram sampling";
    int64_t N = 1<<20;  
    auto xs = Grappa::global_alloc<int64_t>(N);
    forall(xs, N, [N](int64_t i, int64_t& x) { x = rand() % N; });
  
    on_all_cores([xs,N]{
      for (int64_t i=0; i<N; i++) {
        rand_msg = delegate::read(xs+i);
      }
    });
#endif

    on_all_cores([] {
        maz.add( (Grappa::mycore()+Grappa::cores()-1)%Grappa::cores() );
        maz.add( 1 );
    });

    Metrics::stop_tracing();
    Metrics::merge_and_print();
    //Metrics::dump_stats_blob();
  
    call_on_all_cores([]{ Metrics::reset(); });
    Metrics::merge_and_print();
  });
  Grappa::finalize();
}

BOOST_AUTO_TEST_SUITE_END();
