
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include "Delegate.hpp"
#include "Timestamp.hpp"
#include "common.hpp"

#include <cassert>
#include <numeric>
#include <limits>

#include <gflags/gflags.h>
#include <glog/logging.h>

GRAPPA_DEFINE_STAT(HistogramStatistic, delegate_op_latency_histogram, 0);

GRAPPA_DEFINE_STAT(SummarizingStatistic<uint64_t>, flat_combiner_fetch_and_add_amount, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, delegate_short_circuits, 0);

GRAPPA_DEFINE_STAT(SummarizingStatistic<double>, delegate_roundtrip_latency, 0.0);
GRAPPA_DEFINE_STAT(SummarizingStatistic<double>, delegate_network_latency, 0.0);
GRAPPA_DEFINE_STAT(SummarizingStatistic<double>, delegate_wakeup_latency, 0.0);

GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, delegate_ops, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, delegate_targets, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, delegate_reads, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, delegate_read_targets, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, delegate_writes, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, delegate_write_targets, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, delegate_cmpswaps, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, delegate_cmpswap_targets, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, delegate_fetchadds, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, delegate_fetchadd_targets, 0);
