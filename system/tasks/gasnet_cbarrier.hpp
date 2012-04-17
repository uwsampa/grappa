#ifndef __GASNET_CBARRIER__
#define __GASNET_CBARRIER__

void cbarrier_cancel();
int cbarrier_wait();
void cbarrier_init(int num_nodes, int rank);


#endif
