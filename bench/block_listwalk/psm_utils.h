
#ifndef __PSM_UTILS_H__
#define __PSM_UTILS_H__

#include <psm.h>
#include <stdint.h>

#define PSM_CHECK(stmt)							\
  do {									\
    int err = 0;							\
  if( (err = (stmt)) != PSM_OK ) {					\
  LOG_ERROR(__FILE__ ":%d: error: " #stmt " failed (returned %d).\n", __LINE__, err); \
  LOG_ERROR(__FILE__ ":%d: PSM error: %s\n", __LINE__, psm_error_get_string( err ) ); \
  exit(EXIT_FAILURE);							\
  }									\
  } while (0)

psm_epaddr_t dest2epaddr( int dest );
psm_ep_t get_ep();
void psm_setup( int gasneti_mynode, int gasneti_nodes );
void psm_teardown();

void psm_get_bytes_inout( uint64_t * tx_bytes, uint64_t * rx_bytes );

#endif
