
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>

#include <uuid/uuid.h>

#include <psm.h>
#include <psm_mq.h>

#define CHECK(err) if( err != PSM_OK ) { printf("error: %s\n", psm_error_get_string( err ) ); exit(1); }

int main( int argc, char * argv[] ) {

  /*
   * steps to using psm:
   *
   * psm_init - initialize the PSM library
   * psm_uuid_generate(uuid) - generate unique global job ID
   * OOB broadcast of the UUID to every process
   * psm_ep_open(uuid, ep, myepid) - open local endpoint returns ep and myepid
   * psm_mq_init(ep,mq) - initialize local MQ interface
   * OOB exchange of epids into rm eps
   * psm_ep_connect(ep,rm eps,epaddrs) - connects the local endpoint to end-points in rm eps and returns epaddrs
   * psm_mq_irecv(mq,TAG,in,size,reqr) - posts non-blocking receive request for message with TAG
   * psm_mq_send(mq,epaddrs[i],TAG,out,size,reqs) - send content of out buffer with TAG to epaddrs[i]
   * psm_mq_wait(reqr) - wait for receive to finish
   * psm_mq_finalize(mq) - close MQ
   * psm_ep_close(ep) - close EP
   * psm_finalize - close PSM
   */

  int api_verno_major = PSM_VERNO_MAJOR;
  int api_verno_minor = PSM_VERNO_MINOR;

  psm_error_t err;

  printf("Starting.\n");

  printf("Initializing.\n");
  err = psm_init( &api_verno_major, &api_verno_minor );
  CHECK( err );

  char my_uuid_str[37] = { 0 };
  uuid_t my_uuid;
  uuid_generate( my_uuid );
  uuid_unparse( my_uuid, my_uuid_str );
  printf("My UUID is %s\n", my_uuid_str );

  struct psm_ep_open_opts opts = {
    .timeout = -1,
    .unit = -1,
    .affinity = PSM_EP_OPEN_AFFINITY_SKIP,
    .shm_mbytes = -1,
    .sendbufs_num = 128,
    .network_pkey = PSM_EP_OPEN_PKEY_DEFAULT,
    .port = 2,
    .outsl = -1,
    .service_id = -1,
    .path_res_type = -1,
    .senddesc_num = 128,
    .imm_size = 1024,
  };

  psm_ep_t ep;
  psm_epid_t epid;
  printf("Opening EP\n");
  err = psm_ep_open( my_uuid,
  		     NULL, //&opts,
  		     &ep,
  		     &epid );
  CHECK( err );

  psm_mq_t mq;
  printf("Opening MQ\n");
  err = psm_mq_init( ep, PSM_MQ_ORDERMASK_NONE, NULL, 0, &mq );
  CHECK( err );
  
  /*
   * psm_mq_irecv(mq,TAG,in,size,reqr) - posts non-blocking receive request for message with TAG
   * psm_mq_send(mq,epaddrs[i],TAG,out,size,reqs) - send content of out buffer with TAG to epaddrs[i]
   */

  printf("Connecting\n");
  psm_epid_t epids[1] = { epid };
  int epid_masks[1] = { 1 };
  psm_error_t errors[1];
  psm_epaddr_t epaddrs[1];
  err = psm_ep_connect( ep, 1, epids,
		       epid_masks, errors, 
		       epaddrs, 1024 );
  CHECK( err );

  psm_mq_req_t request;
  uint64_t recv_data = 0;
  printf("posting receive\n");
  err = psm_mq_irecv( mq, 0, 0, 0, &recv_data, sizeof(recv_data), NULL, &request );
  CHECK( err );

  uint64_t send_data = 12345;
  printf("posting send\n");
  err = psm_mq_send( mq, epaddrs[0], 0, 0, &send_data, sizeof(send_data) );
  CHECK( err );

  printf("Waiting on request\n");
  err = psm_mq_wait( request, NULL ); //status );
  CHECK( err );

  printf("Send data: %ld receive data: %ld\n", send_data, recv_data );
  assert( send_data == recv_data );

  printf("Finalizing MQ\n");
  err = psm_mq_finalize( mq );
  CHECK( err );

  printf("Closing EP\n");
  err = psm_ep_close( ep, PSM_EP_CLOSE_GRACEFUL, 1024 );
  CHECK( err );

  printf("Finalizing.\n");
  err = psm_finalize();
  CHECK( err );
  printf("Done.\n");

  return 0;
}
