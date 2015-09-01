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

#include "MaxMetric.hpp"
#include "MaxMetricImpl.hpp"

namespace Grappa {

#ifdef VTRACE_SAMPLED
  template <> void MaxMetric<int>::vt_sample() const {
    VT_COUNT_SIGNED_VAL(vt_counter, value_);
  }
  template <> void MaxMetric<int64_t>::vt_sample() const {
    VT_COUNT_SIGNED_VAL(vt_counter, value_);
  }
  template <> void MaxMetric<unsigned>::vt_sample() const {
    VT_COUNT_UNSIGNED_VAL(vt_counter, value_);
  }
  template <> void MaxMetric<uint64_t>::vt_sample() const {
    VT_COUNT_UNSIGNED_VAL(vt_counter, value_);
  }
  template <> void MaxMetric<double>::vt_sample() const {
    VT_COUNT_DOUBLE_VAL(vt_counter, value_);
  }
  template <> void MaxMetric<float>::vt_sample() const {
    VT_COUNT_DOUBLE_VAL(vt_counter, value_);
  }
  
  template <> const int MaxMetric<int>::vt_type = VT_COUNT_TYPE_SIGNED;
  template <> const int MaxMetric<int64_t>::vt_type = VT_COUNT_TYPE_SIGNED;
  template <> const int MaxMetric<unsigned>::vt_type = VT_COUNT_TYPE_UNSIGNED;
  template <> const int MaxMetric<uint64_t>::vt_type = VT_COUNT_TYPE_UNSIGNED;
  template <> const int MaxMetric<double>::vt_type = VT_COUNT_TYPE_DOUBLE;
  template <> const int MaxMetric<float>::vt_type = VT_COUNT_TYPE_FLOAT;
#endif

  // force instantiation of merge_all()
  template void MaxMetric<int>::merge_all(impl::MetricBase*);
  template void MaxMetric<int64_t>::merge_all(impl::MetricBase*);
  template void MaxMetric<unsigned>::merge_all(impl::MetricBase*);
  template void MaxMetric<uint64_t>::merge_all(impl::MetricBase*);
  template void MaxMetric<double>::merge_all(impl::MetricBase*);
  template void MaxMetric<float>::merge_all(impl::MetricBase*);
}

