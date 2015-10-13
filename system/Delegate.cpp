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

#include "Delegate.hpp"
#include "Timestamp.hpp"
#include "common.hpp"

#include <cassert>
#include <numeric>
#include <limits>

#include <gflags/gflags.h>
#include <glog/logging.h>

GRAPPA_DEFINE_METRIC(HistogramMetric, delegate_op_latency_histogram, 0);

GRAPPA_DEFINE_METRIC(SummarizingMetric<uint64_t>, flat_combiner_fetch_and_add_amount, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, delegate_short_circuits, 0);

GRAPPA_DEFINE_METRIC(SummarizingMetric<double>, delegate_roundtrip_latency, 0.0);
GRAPPA_DEFINE_METRIC(SummarizingMetric<double>, delegate_network_latency, 0.0);
GRAPPA_DEFINE_METRIC(SummarizingMetric<double>, delegate_wakeup_latency, 0.0);

GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, delegate_ops, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, delegate_targets, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, delegate_reads, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, delegate_read_targets, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, delegate_writes, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, delegate_write_targets, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, delegate_cmpswaps, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, delegate_cmpswap_targets, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, delegate_fetchadds, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, delegate_fetchadd_targets, 0);
