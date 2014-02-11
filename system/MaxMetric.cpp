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

