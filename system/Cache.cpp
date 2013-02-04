
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

/// Main Explicit Cache API

#include "Cache.hpp"

/// TODO: delete me.
Node address2node( void * ) {
  return 0;
}

CacheStatistics cache_stats;

CacheStatistics::CacheStatistics()
#ifdef VTRACE_SAMPLED
  : cache_grp_vt( VT_COUNT_GROUP_DEF( "Cache" ) )
  , ro_acquires_ev_vt( VT_COUNT_DEF( "RO acquires", "roacquires", VT_COUNT_TYPE_UNSIGNED, cache_grp_vt ) )
  , wo_releases_ev_vt( VT_COUNT_DEF( "WO releases", "woreleases", VT_COUNT_TYPE_UNSIGNED, cache_grp_vt ) )
  , rw_acquires_ev_vt( VT_COUNT_DEF( "RW acquires", "rwacquires", VT_COUNT_TYPE_UNSIGNED, cache_grp_vt ) )
  , rw_releases_ev_vt( VT_COUNT_DEF( "RW releases", "rwreleases", VT_COUNT_TYPE_UNSIGNED, cache_grp_vt ) )
  , bytes_acquired_ev_vt( VT_COUNT_DEF( "Cache bytes acquired", "bytesacq", VT_COUNT_TYPE_UNSIGNED, cache_grp_vt ) )
  , bytes_released_ev_vt( VT_COUNT_DEF( "Cache bytes released", "bytesrel", VT_COUNT_TYPE_UNSIGNED, cache_grp_vt ) )
#endif
{
  reset();
}

void CacheStatistics::reset() {
  ro_acquires = 0;
  wo_releases = 0;
  rw_acquires = 0;
  rw_releases = 0;
  bytes_acquired = 0;
  bytes_released = 0;

  incoherent_acquirer_stats.reset();
  incoherent_releaser_stats.reset();
}

void CacheStatistics::dump( std::ostream& o = std::cout, const char * terminator = "" ) {
  o << "   \"CacheStats\": { "
    << "\"ro_acquires\": " << ro_acquires << ", "
    << "\"wo_releases\": " << wo_releases << ", "
    << "\"rw_acquires\": " << rw_acquires << ", "
    << "\"rw_releases\": " << rw_releases << ", "
    << "\"bytes_acquired\": " << bytes_acquired << ", "
    << "\"bytes_released\": " << bytes_released
    << " }" << "," << std::endl;
  
  incoherent_acquirer_stats.dump( o, ",");
  incoherent_releaser_stats.dump( o, terminator );
}

void CacheStatistics::sample() {
  ;

  incoherent_acquirer_stats.sample();
  incoherent_releaser_stats.sample();
}

void CacheStatistics::profiling_sample() {
#ifdef VTRACE_SAMPLED
  VT_COUNT_UNSIGNED_VAL( ro_acquires_ev_vt, ro_acquires );
  VT_COUNT_UNSIGNED_VAL( wo_releases_ev_vt, wo_releases );
  VT_COUNT_UNSIGNED_VAL( rw_acquires_ev_vt, rw_acquires );
  VT_COUNT_UNSIGNED_VAL( rw_releases_ev_vt, rw_releases );
  VT_COUNT_UNSIGNED_VAL( bytes_acquired_ev_vt, bytes_acquired );
  VT_COUNT_UNSIGNED_VAL( bytes_released_ev_vt, bytes_released );
#endif

  incoherent_acquirer_stats.profiling_sample();
  incoherent_releaser_stats.profiling_sample();
}

void CacheStatistics::merge(CacheStatistics * other) {
  ro_acquires += other->ro_acquires;
  wo_releases += other->wo_releases;
  rw_acquires += other->rw_acquires;
  rw_releases += other->rw_releases;
  bytes_acquired += other->bytes_acquired;
  bytes_released += other->bytes_released;
}

extern uint64_t merge_reply_count;
void CacheStatistics::merge_am(CacheStatistics * other, size_t sz, void* payload, size_t psz) {
  cache_stats.merge( other );
  merge_reply_count++;
}
