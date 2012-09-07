
#include "Cache.hpp"

Node address2node( void * ) {
  return 0;
}



// void incoherent_release_request_am( memory_write_reply_args * args, size_t size, void * payload, size_t payload_size ) {
//   args->descriptor->done = true;
//   Delegate_wakeup( args->descriptor );
// }

// void incoherent_release_reply_am( memory_write_reply_args * args, size_t size, void * payload, size_t payload_size ) {
//   args->descriptor->done = true;
//   Delegate_wakeup( args->descriptor );
// }

// template< typename T >
// void IncoherentReleaser::start_release() { 
//   if( !release_started_ ) {
//     release_started_ = true;
//     ;
//   }
// }

// template< typename T >
// void IncoherentReleaser::block_until_released() {
//     if( !released_ ) {
//       ;
//       released_ = true;
//     }
//   }

// template< typename T >
// bool IncoherentReleaser::released() {
//   return released_;
// }


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
}

void CacheStatistics::dump() {
  std::cout << "CacheStats { "
	    << "ro_acquires: " << ro_acquires << ", "
	    << "wo_releases: " << wo_releases << ", "
	    << "rw_acquires: " << rw_acquires << ", "
	    << "rw_releases: " << rw_releases << ", "
	    << "bytes_acquired: " << bytes_acquired << ", "
	    << "bytes_released: " << bytes_released << ", "
    << " }" << std::endl;
  
}

void CacheStatistics::sample() {
  ;
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
}

void CacheStatistics::merge(const CacheStatistics * other) {
  ro_acquires += other->ro_acquires;
  wo_releases += other->wo_releases;
  rw_acquires += other->rw_acquires;
  rw_releases += other->rw_releases;
  bytes_acquired += other->bytes_acquired;
  bytes_released += other->bytes_released;
}

