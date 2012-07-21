#include "IncoherentAcquirer.hpp"


IAStatistics incoherent_acquirer_stats;


IAStatistics::IAStatistics()
#ifdef VTRACE_SAMPLED
  : ia_grp_vt( VT_COUNT_GROUP_DEF( "IncoherentAcquirer" ) )
  , acquire_ams_ev_vt( VT_COUNT_DEF( "IA acquire ams", "iaams", VT_COUNT_TYPE_UNSIGNED, ia_grp_vt ) )
  , acquire_ams_bytes_ev_vt( VT_COUNT_DEF( "IA acquire ams bytes", "iams_bytes", VT_COUNT_TYPE_UNSIGNED, ia_grp_vt ) )
  , acquire_blocked_ev_vt( VT_COUNT_DEF( "IA blocked op count", "operations", VT_COUNT_TYPE_UNSIGNED, ia_grp_vt ) )
  , acquire_blocked_ticks_total_ev_vt( VT_COUNT_DEF( "IA total blocked ticks", "ticks", VT_COUNT_TYPE_UNSIGNED, ia_grp_vt ) )
  , acquire_blocked_ticks_min_ev_vt( VT_COUNT_DEF( "IA min blocked ticks", "ticks", VT_COUNT_TYPE_UNSIGNED, ia_grp_vt ) )
  , acquire_blocked_ticks_max_ev_vt( VT_COUNT_DEF( "IA max blocked ticks", "ticks", VT_COUNT_TYPE_UNSIGNED, ia_grp_vt ) )
  , average_latency_ev_vt( VT_COUNT_DEF( "IA average latency", "ticks/s", VT_COUNT_TYPE_DOUBLE, ia_grp_vt ) )
  , acquire_network_ticks_total_ev_vt( VT_COUNT_DEF( "IA total network ticks", "ticks", VT_COUNT_TYPE_UNSIGNED, ia_grp_vt ) )
  , acquire_network_ticks_min_ev_vt( VT_COUNT_DEF( "IA min network ticks", "ticks", VT_COUNT_TYPE_UNSIGNED, ia_grp_vt ) )
  , acquire_network_ticks_max_ev_vt( VT_COUNT_DEF( "IA max network ticks", "ticks", VT_COUNT_TYPE_UNSIGNED, ia_grp_vt ) )
  , average_network_latency_ev_vt( VT_COUNT_DEF( "IA average network latency", "ticks/s", VT_COUNT_TYPE_DOUBLE, ia_grp_vt ) )
  , acquire_network_ticks_total_ev_vt( VT_COUNT_DEF( "IA total wakeup ticks", "ticks", VT_COUNT_TYPE_UNSIGNED, ia_grp_vt ) )
  , acquire_wakeup_ticks_min_ev_vt( VT_COUNT_DEF( "IA min wakeup ticks", "ticks", VT_COUNT_TYPE_UNSIGNED, ia_grp_vt ) )
  , acquire_wakeup_ticks_max_ev_vt( VT_COUNT_DEF( "IA max wakeup ticks", "ticks", VT_COUNT_TYPE_UNSIGNED, ia_grp_vt ) )
  , average_wakeup_latency_ev_vt( VT_COUNT_DEF( "IA average wakeup latency", "ticks/s", VT_COUNT_TYPE_DOUBLE, ia_grp_vt ) )
#endif
{
  reset();
}


void IAStatistics::reset() {
  acquire_ams = 0;
  acquire_ams_bytes = 0;
  acquire_blocked = 0;
  acquire_blocked_ticks_total = 0;
  acquire_blocked_ticks_min = std::numeric_limits<uint64_t>::max();
  acquire_blocked_ticks_max = std::numeric_limits<uint64_t>::min();
  acquire_network_ticks_total = 0;
  acquire_network_ticks_min = std::numeric_limits<uint64_t>::max();
  acquire_network_ticks_max = std::numeric_limits<uint64_t>::min();
  acquire_wakeup_ticks_total = 0;
  acquire_wakeup_ticks_min = std::numeric_limits<uint64_t>::max();
  acquire_wakeup_ticks_max = std::numeric_limits<uint64_t>::min();
}

void IAStatistics::dump() {
  std::cout << "IncoherentAcquirerStatistics { "
	    << "acquire_ams: " << acquire_ams << ", "
	    << "acquire_ams_bytes: " << acquire_ams_bytes << ", "
	    << "acquire_blocked: " << acquire_blocked  << ", "
	    << "acquire_blocked_ticks_total: " << acquire_blocked_ticks_total  << ", "
	    << "acquire_blocked_ticks_min: " << acquire_blocked_ticks_min  << ", "
	    << "acquire_blocked_ticks_max: " << acquire_blocked_ticks_max  << ", "
	    << "acquire_average_latency: " << (double) acquire_blocked_ticks_total / acquire_blocked  << ", "
	    << "acquire_network_ticks_total: " << acquire_network_ticks_total  << ", "
	    << "acquire_network_ticks_min: " << acquire_network_ticks_min  << ", "
	    << "acquire_network_ticks_max: " << acquire_network_ticks_max  << ", "
	    << "acquire_average_network_latency: " << (double) acquire_network_ticks_total / acquire_blocked  << ", "
	    << "acquire_wakeup_ticks_total: " << acquire_wakeup_ticks_total  << ", "
	    << "acquire_wakeup_ticks_min: " << acquire_wakeup_ticks_min  << ", "
	    << "acquire_wakeup_ticks_max: " << acquire_wakeup_ticks_max  << ", "
	    << "acquire_average_wakeup_latency: " << (double) acquire_wakeup_ticks_total / acquire_blocked  << ", "
    << " }" << std::endl;
}

void IAStatistics::sample() {
  ;
}

void IAStatistics::profiling_sample() {
#ifdef VTRACE_SAMPLED
  VT_COUNT_UNSIGNED_VAL( acquire_ams_ev_vt, acquire_ams );
  VT_COUNT_UNSIGNED_VAL( acquire_ams_bytes_ev_vt, acquire_ams_bytes );
  VT_COUNT_UNSIGNED_VAL( acquire_blocked_ev_vt, acquire_blocked );
  VT_COUNT_UNSIGNED_VAL( acquire_blocked_ticks_total_ev_vt, acquire_blocked_ticks_total );
  VT_COUNT_UNSIGNED_VAL( acquire_blocked_ticks_min_ev_vt, acquire_blocked_ticks_min );
  VT_COUNT_UNSIGNED_VAL( acquire_blocked_ticks_max_ev_vt, acquire_blocked_ticks_max );
  VT_COUNT_DOUBLE_VAL( average_latency_ev_vt, (double) acquire_blocked / acquire_blocked_ticks );
  VT_COUNT_UNSIGNED_VAL( acquire_network_ticks_total_ev_vt, acquire_network_ticks_total );
  VT_COUNT_UNSIGNED_VAL( acquire_network_ticks_min_ev_vt, acquire_network_ticks_min );
  VT_COUNT_UNSIGNED_VAL( acquire_network_ticks_max_ev_vt, acquire_network_ticks_max );
  VT_COUNT_DOUBLE_VAL( average_network_latency_ev_vt, (double) acquire_network / acquire_blocked_ticks );
  VT_COUNT_UNSIGNED_VAL( acquire_wakeup_ticks_total_ev_vt, acquire_wakeup_ticks_total );
  VT_COUNT_UNSIGNED_VAL( acquire_wakeup_ticks_min_ev_vt, acquire_wakeup_ticks_min );
  VT_COUNT_UNSIGNED_VAL( acquire_wakeup_ticks_max_ev_vt, acquire_wakeup_ticks_max );
  VT_COUNT_DOUBLE_VAL( average_wakeup_latency_ev_vt, (double) acquire_wakeup / acquire_blocked_ticks );
#endif
}

void IAStatistics::merge(IAStatistics * other) {
  acquire_ams += other->acquire_ams;
  acquire_ams_bytes += other->acquire_ams_bytes;
  acquire_blocked += other->acquire_blocked;
  acquire_blocked_ticks_total += other->acquire_blocked_ticks_total;
  if( other->acquire_blocked_ticks_min < acquire_blocked_ticks_min )
    acquire_blocked_ticks_min = other->acquire_blocked_ticks_min;
  if( other->acquire_blocked_ticks_max > acquire_blocked_ticks_max )
    acquire_blocked_ticks_max = other->acquire_blocked_ticks_max;
  acquire_network_ticks_total += other->acquire_network_ticks_total;
  if( other->acquire_network_ticks_min < acquire_network_ticks_min )
    acquire_network_ticks_min = other->acquire_network_ticks_min;
  if( other->acquire_network_ticks_max > acquire_network_ticks_max )
    acquire_network_ticks_max = other->acquire_network_ticks_max;
  acquire_wakeup_ticks_total += other->acquire_wakeup_ticks_total;
  if( other->acquire_wakeup_ticks_min < acquire_wakeup_ticks_min )
    acquire_wakeup_ticks_min = other->acquire_wakeup_ticks_min;
  if( other->acquire_wakeup_ticks_max > acquire_wakeup_ticks_max )
    acquire_wakeup_ticks_max = other->acquire_wakeup_ticks_max;
}

