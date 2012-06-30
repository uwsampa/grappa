#include "IncoherentAcquirer.hpp"


IAStatistics incoherent_acquirer_stats;


IAStatistics::IAStatistics()
#ifdef VTRACE_SAMPLED
  : ia_grp_vt( VT_COUNT_GROUP_DEF( "IncoherentAcquirer" ) )
  , acquire_ams_ev_vt( VT_COUNT_DEF( "IA acquire ams", "iaams", VT_COUNT_TYPE_UNSIGNED, ia_grp_vt ) )
  , acquire_ams_bytes_ev_vt( VT_COUNT_DEF( "IA acquire ams bytes", "iams_bytes", VT_COUNT_TYPE_UNSIGNED, ia_grp_vt ) )
#endif
{
  reset();
}


void IAStatistics::reset() {
  acquire_ams = 0;
  acquire_ams_bytes = 0;
}

void IAStatistics::dump() {
  std::cout << "IncoherentAcquirerStatistics { "
	    << "acquire_ams: " << acquire_ams << ", "
	    << "acquire_ams_bytes: " << acquire_ams_bytes << ", "
    << " }" << std::endl;
}

void IAStatistics::sample() {
  ;
}

void IAStatistics::profiling_sample() {
#ifdef VTRACE_SAMPLED
  VT_COUNT_UNSIGNED_VAL( acquire_ams_ev_vt, acquire_ams );
  VT_COUNT_UNSIGNED_VAL( acquire_ams_bytes_ev_vt, acquire_ams_bytes );
#endif
}

void IAStatistics::merge(IAStatistics * other) {
  acquire_ams += other->acquire_ams;
  acquire_ams_bytes += other->acquire_ams_bytes;
}

