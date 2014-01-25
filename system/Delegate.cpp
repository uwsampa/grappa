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

GRAPPA_DEFINE_METRIC(SimpleMetric<int64_t>, delegate_ops_small_msg, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<int64_t>, delegate_ops_payload_msg, 0);
