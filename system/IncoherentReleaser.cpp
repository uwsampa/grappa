
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include "IncoherentReleaser.hpp"
#include "Metrics.hpp"

GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>,  release_ams, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>,  release_ams_bytes, 0);

void IRMetrics::count_release_ams( uint64_t bytes ) {
  release_ams++;
  release_ams_bytes+=bytes;
}
