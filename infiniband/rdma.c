
#include <stdio.h>
#include <assert.h>
#include <memory.h>

#include <netinet/in.h>
#include <rdma/rdma_cma.h>

#include "debug.h"
#include "find_ib_interfaces.h"

static struct rdma_event_channel * ec = NULL;
static struct rdma_cm_id * listener = NULL;







int on_connect_request(struct rdma_cm_id *id)
{
/*   struct rdma_conn_param cm_params; */

/*   LOG_INFO("received connection request.\n"); */
/*   build_connection(id); */
/*   build_params(&cm_params); */
/*   //sprintf(get_local_message_region(id->context), "message from passive/server side with pid %d", getpid()); */
/*   int result = rdma_accept(id, &cm_params); */
/*   assert( 0 == result && "rdma accept failed"); */

  return 0;
}

int on_connection(struct rdma_cm_id *id)
{
/*   on_connect(id->context); */

  return 0;
}





void destroy_connection(void *context)
{
/*   struct connection *conn = (struct connection *)context; */

/*   rdma_destroy_qp(conn->id); */

/*   ibv_dereg_mr(conn->send_mr); */
/*   ibv_dereg_mr(conn->recv_mr); */
/*   ibv_dereg_mr(conn->rdma_local_mr); */
/*   ibv_dereg_mr(conn->rdma_remote_mr); */

/*   free(conn->send_msg); */
/*   free(conn->recv_msg); */
/*   free(conn->rdma_local_region); */
/*   free(conn->rdma_remote_region); */

/*   rdma_destroy_id(conn->id); */

/*   free(conn); */
}

int on_disconnect(struct rdma_cm_id *id)
{
/*   LOG_INFO("peer disconnected.\n"); */

/*   destroy_connection(id->context); */
  return 0;
}

int on_event(struct rdma_cm_event *event)
{
  int r = 0;

  if (event->event == RDMA_CM_EVENT_CONNECT_REQUEST)
    r = on_connect_request(event->id);
  else if (event->event == RDMA_CM_EVENT_ESTABLISHED)
    r = on_connection(event->id);
  else if (event->event == RDMA_CM_EVENT_DISCONNECTED)
    r = on_disconnect(event->id);
  else
    assert( 0 && "on_event: unknown event." );

  return r;
}


static uint16_t setup_server( const char * node_name, struct sockaddr * ib_addr ) {
  int result;

  struct sockaddr_in * ib_addr_in = (struct sockaddr_in *) ib_addr;
  ib_addr_in->sin_port = htons(45275);

  result = rdma_bind_addr( listener, ib_addr );
  assert( 0 == result && "rdma add  r bind failed" );
  
  int connection_backlog = 10;
  result = rdma_listen( listener, connection_backlog );
  assert( 0 == result && "rdma addr bind failed" );
  
  uint16_t port = ntohs( rdma_get_src_port( listener ) );

  LOG_INFO( "%s's RDMA connection manager listening on port %d\n", node_name, port );

  return port;
}

struct ib_connection {
  struct rdma_cm_id * cm_id;
  struct ibv_pd * pd;
  struct ibv_comp_channel * completion_channel;
  struct ibv_cq * completion_queue;
};

static void server_accept_connection(struct ib_connection * connection) {
  int result;

  // accept connection
  result = rdma_get_cm_event(cm_channel, &event);
  assert( 0 == result && "get CM event failed" );
  assert( RDMA_CM_EVENT_CONNECT_REQUEST != event->event && "CM got something other than a connection request");

  connection->cm_id = event->id;

  rdma_ack_cm_event(event);

  connection->pd = ibv_alloc_pd( connection->cm_id->verbs );
  assert( NULL != connection->pd && "pd allocation failed" );

  connection->completion_channel = ibv_create_comp_channel( connection->cm_id->verbs );
  assert( NULL != connection->completion_channel && "completion_channel creation failed" );

  const int completion_queue_entries = 2;
  connection->completion_queue = ibv_create_cq( connection->cm_id->verbs, completion_queue_entries, NULL, connection->completion_channel, 0 );
  assert( NULL != connection->completion_channel && "completion_channel creation failed" );

  result = ibv_req_notify_cq( connection->completion_queue, 0 );
  assert( 0 == result && "completion_queue notify failed" );

  const int outstanding_work_requests = 1;
  const int scatter_gather_elements = 1;
  struct ibv_qp_init_attr qp_attr = { };
  
  qp_attr.cap.max_send_wr = outstanding_work_requests;
  qp_attr.cap.max_recv_wr = outstanding_work_requests;

  qp_attr.cap.max_send_sge = scatter_gather_elements;
  qp_attr.cap.max_recv_sge = scatter_gather_elements;
  
  qp_attr.send_cq = cq;
  qp_attr.recv_cq = cq;
  
  qp_attr.qp_type = IBV_QPT_RC;

  result = rdma_create_qp( connection->cm_id, connection->pd, &qp_attr );
  assert( 0 == result && "queue pair creation failed" );

  struct ibv_sge sge;
	struct ibv_send_wr		send_wr = { };
	struct ibv_send_wr	       *bad_send_wr;
	struct ibv_recv_wr		recv_wr = { };
	struct ibv_recv_wr	       *bad_recv_wr;
	struct ibv_wc			wc;

	return connection;

}


uint16_t rdma_setup(const char * node_name) {
  // get IP address for first infiniband interface
  struct sockaddr ib_addr;
  int count = find_ib_interfaces( &ib_addr, 1 );
  assert(1 == count && "not enough infiniband interfaces");
  LOG_INFO("%s's infiniband IP address: %s\n", node_name, inet_ntoa( ((struct sockaddr_in *)&ib_addr)->sin_addr ) );

  ec = rdma_create_event_channel();
  assert( NULL != ec && "no event channel found" );

  int result;

  result = rdma_create_id( ec, &listener, NULL, RDMA_PS_TCP );
  assert( 0 == result && "rdma id creation failed" );

  uint16_t server_port = setup_server( node_name, &ib_addr );

  //setup_client( &ec, &listener, &ib_addr );




  struct rdma_cm_event *event = NULL;
  while (rdma_get_cm_event(ec, &event) == 0) {
    struct rdma_cm_event event_copy;

    memcpy(&event_copy, event, sizeof(*event));
    rdma_ack_cm_event(event);

    if (on_event(&event_copy))
      break;
  }

  return server_port;
}



void rdma_finalize() {
  rdma_destroy_id( listener );
  rdma_destroy_event_channel( ec );
}
