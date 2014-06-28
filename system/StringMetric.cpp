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

#include "StringMetric.hpp"
#include "StringMetricImpl.hpp"

namespace Grappa {

#ifdef VTRACE_SAMPLED
  void StringMetric::vt_sample() const {
    // TODO: for now, strings are traced as -1 no value
    VT_COUNT_SIGNED_VAL(vt_counter, -1);
  }

  // TODO: see TODO above  
  const int StringMetric::vt_type = VT_COUNT_TYPE_SIGNED;
#endif

  // force instantiation of merge_all()
  void StringMetric::merge_all(impl::MetricBase*);

}

