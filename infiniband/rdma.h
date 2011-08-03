
#ifndef __RDMA_H__
#define __RDMA_H__

struct connection {
  struct rdma_cm_id *id;
  struct ibv_qp *qp;

  int connected;

};


void rdma_setup(const char * node_name);
void rdma_finalize();

#endif
