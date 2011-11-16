
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include <psm.h>
#include <psm_mq.h>
#include <psm_am.h>

#include "psm_utils.h"

#define GASNET_CONDUIT_IBV 1
#include <gasnet.h>
#include <gasnet_bootstrap_internal.h>

#include "debug.h"

static psm_ep_t ep;
static psm_epid_t * epids; 
static psm_epaddr_t * epaddrs;

psm_epaddr_t dest2epaddr( int dest ) {
  return epaddrs[dest];
}

psm_ep_t get_ep() {
  return ep;
}

void psm_setup( int gasneti_mynode, int gasneti_nodes ) {
  int i;

  // start up PSM library
  //printf("starting up PSM on node %d on %s\n", gasneti_mynode, gasneti_gethostname() );
  int api_verno_major = PSM_VERNO_MAJOR;
  int api_verno_minor = PSM_VERNO_MINOR;
  PSM_CHECK( psm_init( &api_verno_major, &api_verno_minor ) );

  // compute uuid
  psm_uuid_t job_uuid;
  if( 0 == gasnet_mynode() ) {
    psm_uuid_generate( job_uuid );
  }
  gasneti_bootstrapBroadcast_ssh( &job_uuid, sizeof( job_uuid ), &job_uuid, 0 );

  char job_uuid_str[37] = { 0 };
  uuid_unparse( job_uuid, job_uuid_str );
  LOG_INFO("Node %d says job UUID is %s\n", gasneti_mynode, job_uuid_str );

  // set up endpoints
  //psm_ep_t ep;
  //psm_epid_t epids[ gasneti_nodes ];
  epids = malloc( gasneti_nodes * sizeof(psm_epid_t) );

  struct psm_ep_open_opts epopts;
  psm_ep_open_opts_get_defaults( &epopts );
  /* epopts.affinity = PSM_EP_OPEN_AFFINITY_SKIP; */
  /* epopts.unit = gasneti_mynode % half; */
  /* epopts.network_pkey = 12345; */
  epopts.port = 2;
  /* PSM_CHECK( psm_ep_open( job_uuid, */
  /* 			  &epopts, */
  /* 			  &ep, */
  /* 			  &epids[ gasneti_mynode ] ) ); */
  int open_count = 5;
  psm_error_t open_err = 0;
  do {
    open_err = psm_ep_open( job_uuid,
			    &epopts,
			    &ep,
			    &epids[ gasneti_mynode ] );
  } while ( open_err != 0 && 0 < open_count-- );
  PSM_CHECK( open_err );
  gasneti_bootstrapExchange_ssh( &epids[ gasneti_mynode ], sizeof( psm_epid_t ), epids );

  //if( 0 == gasneti_mynode ) {
  for( i = 0; i < gasneti_nodes; ++i ) {
    printf( "Node %d (ep %d, pid %d) knows that node %d has epid %lu\n", gasneti_mynode, ep, getpid(), i, epids[i] ); 
  }
  //}

  gasneti_bootstrapBarrier_ssh();

  psm_error_t errors[ gasneti_nodes ];
  //psm_epaddr_t epaddrs[ gasneti_nodes ];
  epaddrs = malloc( gasneti_nodes * sizeof(psm_epaddr_t) );
  printf("Node %d connecting.\n", gasneti_mynode ); fflush(stdout);

  int err = 0;
  err = psm_ep_connect( ep, gasneti_nodes, epids, NULL,
			errors, epaddrs, 3e9 );
  if (err != PSM_OK) {
    for( i = 0; i < gasneti_nodes; ++i ) {
      printf( "Node %d connection to node %d has error %d\n", gasneti_mynode, i, errors[i] ); 
    }
  }
  PSM_CHECK( err );

  printf("Node %d connected.\n", gasneti_mynode ); //fflush(stdout);
  gasneti_bootstrapBarrier_ssh();
}


void psm_teardown() {
  gasneti_bootstrapBarrier_ssh();
  PSM_CHECK( psm_ep_close( ep, PSM_EP_CLOSE_GRACEFUL, 0 ) );
  PSM_CHECK( psm_finalize() );
  free(epids);
  free(epaddrs);
}

static uint64_t psm_fixup_value( uint64_t value, char modifier ) {
  switch( modifier ) {
  case 'K': value *= 1000; break;
  case 'M': value *= 1000000; break;
  case 'B':
  case 'G': value *= 1000000000; break;
  default: break;
  }
 return value * 4;
}

void psm_get_bytes_inout( uint64_t * tx_bytes, uint64_t * rx_bytes ) {
  FILE *fp;  
  
  fp = popen("/usr/bin/ipathstats -1 -i 0:2 | grep Words", "r");
  if (fp == NULL) {
    printf("Failed to run ipathstats\n" );
    exit;
  }
  
  uint64_t value = 0;
  char modifier[2];
  assert( 2 == fscanf( fp, " TxWords %lu%[KMBG]\n", &value, &modifier ) );
  *tx_bytes = psm_fixup_value( value, modifier );
  assert( 2 == fscanf( fp, " RxWords %lu%[KMBG]\n", &value, &modifier ) );
  *rx_bytes = psm_fixup_value( value, modifier );

  pclose(fp);
}
