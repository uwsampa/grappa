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

#include "SummarizingMetric.hpp"
#include "SummarizingMetricImpl.hpp"

namespace Grappa {

#ifdef VTRACE_SAMPLED
  template <> void SummarizingMetric<int>::vt_sample() const {
    VT_COUNT_SIGNED_VAL(vt_counter_value, value_);
    VT_COUNT_UNSIGNED_VAL(vt_counter_count, n);
    VT_COUNT_DOUBLE_VAL(vt_counter_mean, mean);
    VT_COUNT_DOUBLE_VAL(vt_counter_stddev, stddev());
  }
  template <> void SummarizingMetric<int64_t>::vt_sample() const {
    VT_COUNT_SIGNED_VAL(vt_counter_value, value_);
    VT_COUNT_UNSIGNED_VAL(vt_counter_count, n);
    VT_COUNT_DOUBLE_VAL(vt_counter_mean, mean);
    VT_COUNT_DOUBLE_VAL(vt_counter_stddev, stddev());
  }
  template <> void SummarizingMetric<unsigned>::vt_sample() const {
    VT_COUNT_UNSIGNED_VAL(vt_counter_value, value_);
    VT_COUNT_UNSIGNED_VAL(vt_counter_count, n);
    VT_COUNT_DOUBLE_VAL(vt_counter_mean, mean);
    VT_COUNT_DOUBLE_VAL(vt_counter_stddev, stddev());
  }
  template <> void SummarizingMetric<uint64_t>::vt_sample() const {
    VT_COUNT_UNSIGNED_VAL(vt_counter_value, value_);
    VT_COUNT_UNSIGNED_VAL(vt_counter_count, n);
    VT_COUNT_DOUBLE_VAL(vt_counter_mean, mean);
    VT_COUNT_DOUBLE_VAL(vt_counter_stddev, stddev());
  }
  template <> void SummarizingMetric<double>::vt_sample() const {
    VT_COUNT_DOUBLE_VAL(vt_counter_value, value_);
    VT_COUNT_UNSIGNED_VAL(vt_counter_count, n);
    VT_COUNT_DOUBLE_VAL(vt_counter_mean, mean);
    VT_COUNT_DOUBLE_VAL(vt_counter_stddev, stddev());
  }
  template <> void SummarizingMetric<float>::vt_sample() const {
    VT_COUNT_DOUBLE_VAL(vt_counter_value, value_);
    VT_COUNT_UNSIGNED_VAL(vt_counter_count, n);
    VT_COUNT_DOUBLE_VAL(vt_counter_mean, mean);
    VT_COUNT_DOUBLE_VAL(vt_counter_stddev, stddev());
  }
  
  template <> const int SummarizingMetric<int>::vt_type = VT_COUNT_TYPE_SIGNED;
  template <> const int SummarizingMetric<int64_t>::vt_type = VT_COUNT_TYPE_SIGNED;
  template <> const int SummarizingMetric<unsigned>::vt_type = VT_COUNT_TYPE_UNSIGNED;
  template <> const int SummarizingMetric<uint64_t>::vt_type = VT_COUNT_TYPE_UNSIGNED;
  template <> const int SummarizingMetric<double>::vt_type = VT_COUNT_TYPE_DOUBLE;
  template <> const int SummarizingMetric<float>::vt_type = VT_COUNT_TYPE_FLOAT;
#endif

  // force instantiation of merge_all()
  template void SummarizingMetric<int>::merge_all(impl::MetricBase*);
  template void SummarizingMetric<int64_t>::merge_all(impl::MetricBase*);
  template void SummarizingMetric<unsigned>::merge_all(impl::MetricBase*);
  template void SummarizingMetric<uint64_t>::merge_all(impl::MetricBase*);
  template void SummarizingMetric<double>::merge_all(impl::MetricBase*);
  template void SummarizingMetric<float>::merge_all(impl::MetricBase*);
}

