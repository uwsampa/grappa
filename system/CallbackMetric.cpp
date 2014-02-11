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

#include "CallbackMetric.hpp"
#include "CallbackMetricImpl.hpp"

namespace Grappa {

#ifdef VTRACE_SAMPLED
  template <> void CallbackMetric<int>::vt_sample() const {
    VT_COUNT_SIGNED_VAL(vt_counter, value());
  }
  template <> void CallbackMetric<int64_t>::vt_sample() const {
    VT_COUNT_SIGNED_VAL(vt_counter, value());
  }
  template <> void CallbackMetric<unsigned>::vt_sample() const {
    VT_COUNT_UNSIGNED_VAL(vt_counter, value());
  }
  template <> void CallbackMetric<uint64_t>::vt_sample() const {
    VT_COUNT_UNSIGNED_VAL(vt_counter, value());
  }
  template <> void CallbackMetric<double>::vt_sample() const {
    VT_COUNT_DOUBLE_VAL(vt_counter, value());
  }
  template <> void CallbackMetric<float>::vt_sample() const {
    VT_COUNT_DOUBLE_VAL(vt_counter, value());
  }
  
  template <> const int CallbackMetric<int>::vt_type = VT_COUNT_TYPE_SIGNED;
  template <> const int CallbackMetric<int64_t>::vt_type = VT_COUNT_TYPE_SIGNED;
  template <> const int CallbackMetric<unsigned>::vt_type = VT_COUNT_TYPE_UNSIGNED;
  template <> const int CallbackMetric<uint64_t>::vt_type = VT_COUNT_TYPE_UNSIGNED;
  template <> const int CallbackMetric<double>::vt_type = VT_COUNT_TYPE_DOUBLE;
  template <> const int CallbackMetric<float>::vt_type = VT_COUNT_TYPE_FLOAT;
#endif

  // force instantiation of merge_all()
  template void CallbackMetric<int>::merge_all(impl::MetricBase*);
  template void CallbackMetric<int64_t>::merge_all(impl::MetricBase*);
  template void CallbackMetric<unsigned>::merge_all(impl::MetricBase*);
  template void CallbackMetric<uint64_t>::merge_all(impl::MetricBase*);
  template void CallbackMetric<double>::merge_all(impl::MetricBase*);
  template void CallbackMetric<float>::merge_all(impl::MetricBase*);
}

