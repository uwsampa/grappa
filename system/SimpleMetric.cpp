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

#include "SimpleMetric.hpp"
#include "SimpleMetricImpl.hpp"

namespace Grappa {

#ifdef VTRACE_SAMPLED
  template <> void SimpleMetric<int>::vt_sample() const {
    VT_COUNT_SIGNED_VAL(vt_counter, value_);
  }
  template <> void SimpleMetric<int64_t>::vt_sample() const {
    VT_COUNT_SIGNED_VAL(vt_counter, value_);
  }
  template <> void SimpleMetric<unsigned>::vt_sample() const {
    VT_COUNT_UNSIGNED_VAL(vt_counter, value_);
  }
  template <> void SimpleMetric<uint64_t>::vt_sample() const {
    VT_COUNT_UNSIGNED_VAL(vt_counter, value_);
  }
  template <> void SimpleMetric<double>::vt_sample() const {
    VT_COUNT_DOUBLE_VAL(vt_counter, value_);
  }
  template <> void SimpleMetric<float>::vt_sample() const {
    VT_COUNT_DOUBLE_VAL(vt_counter, value_);
  }
  
  template <> const int SimpleMetric<int>::vt_type = VT_COUNT_TYPE_SIGNED;
  template <> const int SimpleMetric<int64_t>::vt_type = VT_COUNT_TYPE_SIGNED;
  template <> const int SimpleMetric<unsigned>::vt_type = VT_COUNT_TYPE_UNSIGNED;
  template <> const int SimpleMetric<uint64_t>::vt_type = VT_COUNT_TYPE_UNSIGNED;
  template <> const int SimpleMetric<double>::vt_type = VT_COUNT_TYPE_DOUBLE;
  template <> const int SimpleMetric<float>::vt_type = VT_COUNT_TYPE_FLOAT;
#endif

  // force instantiation of merge_all()
  template void SimpleMetric<int>::merge_all(impl::MetricBase*);
  template void SimpleMetric<int64_t>::merge_all(impl::MetricBase*);
  template void SimpleMetric<unsigned>::merge_all(impl::MetricBase*);
  template void SimpleMetric<uint64_t>::merge_all(impl::MetricBase*);
  template void SimpleMetric<double>::merge_all(impl::MetricBase*);
  template void SimpleMetric<float>::merge_all(impl::MetricBase*);
}

