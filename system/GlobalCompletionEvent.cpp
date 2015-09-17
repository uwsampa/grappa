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

/// The GlobalCompletionEvents master core is defined to be core 0.
const Core GlobalCompletionEvent::master_core = 0;

std::vector<GlobalCompletionEvent*> GlobalCompletionEvent::user_tracked_gces;

std::vector<GlobalCompletionEvent*> GlobalCompletionEvent::get_user_tracked() {
  // returns a copy of the vector to preserve integrity
  return GlobalCompletionEvent::user_tracked_gces;
}

GRAPPA_DEFINE_METRIC(CallbackMetric<int64_t>, gce_user_incomplete, []{
  int64_t sum = 0;
  for (auto g : GlobalCompletionEvent::get_user_tracked()) {
    sum += g->incomplete();
  }
  return sum;
});
