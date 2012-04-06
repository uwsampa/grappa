#ifndef __GASNET_CBARRIER__
#define __GASNET_CBARRIER__

#include <gasnet.h>

#define CANCEL_CBARRIER_REQUEST_HANDLER 222
#define EXIT_CBARRIER_REQUEST_HANDLER 223
#define ENTER_CBARRIER_REQUEST_HANDLER 224
        
void enter_cbarrier_request_handler(gasnet_token_t token);
void exit_cbarrier_request_handler(gasnet_token_t, gasnet_handlerarg_t);
void cancel_cbarrier_request_handler(gasnet_token_t);

void cbarrier_cancel();
int cbarrier_wait();
void cbarrier_init(int num_nodes, int rank);


#endif
