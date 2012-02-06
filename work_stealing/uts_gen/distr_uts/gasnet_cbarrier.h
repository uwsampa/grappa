#ifndef __GASNET_CBARRIER__
#define __GASNET_CBARRIER__

#include <gasnet.h>
        
void enter_cbarrier_request_handler(gasnet_token_t token);
void exit_cbarrier_request_handler(gasnet_token_t, gasnet_handlerarg_t);
void cancel_cbarrier_request_handler(gasnet_token_t);

void cbarrier_cancel();
void cbarrier_wait();
void cbarrier_init(int num_nodes, int rank);


#endif
