#ifndef __ARMCI_ONESIDED_H__
#define __ARMCI_ONESIDED_H__

#include "onesided.h"

#define NUM_SERV_BUFS           1
#define MAX_MEM_REGIONS         30

#define ARMCI_BUF_SIZE          262144
#define ARMCI_SMALL_BUF_SIZE    2048

#define ARMCI_MAX_BUFS          4
#define ARMCI_MAX_SMALL_BUFS    8

#define ARMCI_MAX_DESCRIPTORS   (ARMCI_MAX_BUFS+ARMCI_MAX_SMALL_BUFS)
#define ARMCI_MAX_REQUEST_SIZE  ARMCI_SMALL_BUF_SIZE

#define ARMCI_LIMIT_REMOTE_REQUESTS_BY_NODE

/* typedefs */

typedef struct armci_onesided_msg_tag_s {
        int msgid;
        cos_mdesc_t response_mdesc;
} armci_onesided_msg_tag_t;



/* functions */
int armci_onesided_init();
void armci_transport_cleanup();
void armci_rcv_req(void *,void *,void *,void *,int *);

void print_data(void *);

/* set up internals */

#ifdef MAX_BUFS
#error "MAX_BUFS should not be defined yet"
#else
#define MAX_BUFS                ARMCI_MAX_BUFS
#endif

#ifdef MAX_SMALL_BUFS
#error "MAX_SMALL_BUFS should not be defined yet"
#else
#define MAX_SMALL_BUFS          ARMCI_MAX_SMALL_BUFS
#endif

#ifdef MSG_BUFLEN_DBL
#error "MSG_BUFLEN_DBL should not be defined yet"
#else
#define MSG_BUFLEN_DBL          ARMCI_BUF_SIZE
#endif


/* for buffers */

extern char **client_buf_ptrs;
#define BUF_ALLOCATE armci_portals_client_buf_allocate
//define BUF_EXTRA_FIELD_T comp_desc* 
//define INIT_SEND_BUF(_field,_snd,_rcv) _snd=1;_rcv=1;_field=NULL
#define GET_SEND_BUFFER _armci_buf_get
#define FREE_SEND_BUFFER _armci_buf_release

//define CLEAR_SEND_BUF_FIELD(_field,_snd,_rcv,_to,_op) if((_op==UNLOCK || _op==PUT || ARMCI_ACC(_op)) && _field!=NULL)x_buf_wait_ack((request_header_t *)((void **)&(_field)+1),((char *)&(_field)-sizeof(BUF_INFO_T)));_field=NULL;
//define TEST_SEND_BUF_FIELD(_field,_snd,_rcv,_to,_op,_ret)

#define CLEAR_SEND_BUF_FIELD(_field,_snd,_rcv,_to,_op)
#define TEST_SEND_BUF_FIELD(_field,_snd,_rcv,_to,_op,_ret)

#define COMPLETE_HANDLE _armci_buf_complete_nb_request

//define NB_CMPL_T comp_desc*
#if 0
#define ARMCI_NB_WAIT(_cntr) if(_cntr){\
        int rc;\
        if(nb_handle->tag)\
          if(nb_handle->tag==_cntr->tag)\
          rc = armci_client_complete(0,nb_handle->proc,nb_handle->tag,_cntr);\
} else{\
printf("\n%d:wait null ctr\n",armci_me);}
#endif
#define ARMCI_NB_WAIT(_cntr)



#endif
