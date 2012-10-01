
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include "IncoherentReleaser.hpp"


IRStatistics incoherent_releaser_stats;


IRStatistics::IRStatistics()
#ifdef VTRACE_SAMPLED
  : ir_grp_vt( VT_COUNT_GROUP_DEF( "IncoherentReleaser" ) )
  , release_ams_ev_vt( VT_COUNT_DEF( "IR release ams", "iaams", VT_COUNT_TYPE_UNSIGNED, ir_grp_vt ) )
  , release_ams_bytes_ev_vt( VT_COUNT_DEF( "IR release ams bytes", "irms_bytes", VT_COUNT_TYPE_UNSIGNED, ir_grp_vt ) )
#endif
{
  reset();
}


void IRStatistics::reset() {
  release_ams = 0;
  release_ams_bytes = 0;
}

void IRStatistics::dump() {
  std::cout << "IncoherentReleaserStatistics { "
	    << "release_ams: " << release_ams << ", "
	    << "release_ams_bytes: " << release_ams_bytes << ", "
    << " }" << std::endl;
}

void IRStatistics::sample() {
  ;
}

void IRStatistics::profiling_sample() {
#ifdef VTRACE_SAMPLED
  VT_COUNT_UNSIGNED_VAL( release_ams_ev_vt, release_ams );
  VT_COUNT_UNSIGNED_VAL( release_ams_bytes_ev_vt, release_ams_bytes );
#endif
}

void IRStatistics::merge(IRStatistics * other) {
  release_ams += other->release_ams;
  release_ams_bytes += other->release_ams_bytes;
}

