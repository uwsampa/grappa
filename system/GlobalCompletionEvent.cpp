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

#include "GlobalCompletionEvent.hpp"

using namespace Grappa;

DEFINE_bool(flatten_completions, true, "Flatten GlobalCompletionEvents.");

GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, gce_total_remote_completions, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, gce_completions_sent, 0);

GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, ce_remote_completions, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, ce_completions, 0);

GRAPPA_DEFINE_METRIC(CallbackMetric<int64_t>, gce_incomplete, []{
  return impl::local_gce.incomplete();
});

std::vector<GlobalCompletionEvent&> GlobalCompletionEvent::user_tracked_gces;

std::vector<GlobalCompletionEvent&> GlobalCompletionEvent::get_user_tracked() {
  // returns a copy of the vector to preserve integrity
  return GlobalCompletionEvent::user_tracked_gces;
}

GRAPPA_DEFINE_METRIC(CallbackMetric<int64_t>, gce_app_incomplete, []{
  int64_t sum = 0;
  for (auto g : Grappa::GlobalCompletionEvent::get_user_tracked()) {
    sum += g.incomplete();
  }
});
