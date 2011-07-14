#if HAVE_CONFIG_H
#   include "config.h"
#endif

#include "armcip.h"
int armci_onesided_ds_handler(void *);


#ifdef ARMCI_REGISTER_SHMEM
typedef struct {
       void *base_ptr;
       void *serv_ptr;
       size_t size;
       int islocal;
       int valid;
} aptl_reginfo_t;

typedef struct {
       aptl_reginfo_t reginfo[MAX_MEM_REGIONS];
       int reg_count;
} rem_meminfo_t;

static rem_meminfo_t *_rem_meminfo;
static aptl_reginfo_t *_tmp_rem_reginfo;
#define IN_REGION(_ptr__,_reg__) ((_reg__.valid) && (_ptr__)>=(_reg__.serv_ptr) \
                && (_ptr__) <= ( (char *)(_reg__.serv_ptr)+_reg__.size))
#endif

char **client_buf_ptrs;

int 
armci_onesided_init()
{
        int i;
        cos_parameters_t cos_params;

        cos_params.options        = ONESIDED_DS_PER_NUMA;
        cos_params.nDataServers   = 1;
        cos_params.maxDescriptors = ARMCI_MAX_DESCRIPTORS;
        cos_params.maxRequestSize = ARMCI_MAX_REQUEST_SIZE;
        cos_params.dsHandlerFunc  = armci_onesided_ds_handler;

        COS_Init( &cos_params );

     // initialize armci memory
      # ifdef ARMCI_REGISTER_SHMEM
        _rem_meminfo = (rem_meminfo_t *)calloc(armci_nproc,sizeof(rem_meminfo_t));
        _tmp_rem_reginfo = (aptl_reginfo_t *)malloc(sizeof(aptl_reginfo_t)*armci_nproc);
        if( _rem_meminfo==NULL || _tmp_rem_reginfo ==NULL) {
           armci_die("malloc failed in init_portals",0);
        }
        if(armci_me == 0) {
           printf("sizeof(rem_meminfo_t)=%ld\n",sizeof(rem_meminfo_t));
        }
      # endif
        client_buf_ptrs = (char **) calloc(armci_nproc,sizeof(char *));
        assert(client_buf_ptrs);
        armci_msg_barrier();
        _armci_buf_init();

     // each armci buffer has a cos_request_t associated with it
     // initialize that cos_request_t now
     // moved into the above _armci_buf_init routine
     // for(i=0; i<MAX_BUFS; i++) cpReqCreate(&_armci_buffers[i].id.ar.req);
     // for(i=0; i<MAX_SMALL_BUFS; i++) cpReqCreate(&_armci_smbuffers[i].id.ar.req);

        return 0;
}



void 
armci_transport_cleanup()
{
    /*for i=0tomaxpendingclean*/
    ARMCI_PR_DBG("enter",0);
    free(client_buf_ptrs);
    ARMCI_PR_DBG("exit",0);
} 



static void 
armci_onesided_send(void *buffer, request_header_t *msginfo, int remote_node, cos_request_t *req)
{
        size_t length = sizeof(request_header_t) + msginfo->dscrlen + msginfo->datalen;

     // print_data(msginfo);
        cpReqInit(remote_node, req);
        cpPrePostRecv(buffer, length, req);
        cpCopyLocalDataMDesc(req, &msginfo->tag.response_mdesc);
        if(length > ARMCI_MAX_REQUEST_SIZE) length = sizeof(request_header_t); 
        cpReqSend(msginfo, length, req);
}



void
print_data(void* buf)
{
        request_header_t *msginfo = (request_header_t *) buf;
        char *buffer = (char *) buf;
        buffer += sizeof(request_header_t) + msginfo->dscrlen;

        int ndouble = msginfo->datalen/8;
        double *data = (double *) buffer;

        printf("%d: [0]=%lf; [%d]=%lf; from=%d; to=%d\n",armci_me,data[0],ndouble-1,data[ndouble-1],msginfo->from, msginfo->to);
}



static void 
armci_onesided_recv(void* buffer, request_header_t *msginfo, int remote_node, cos_request_t *req)
{
        size_t length = sizeof(request_header_t) + msginfo->dscrlen;

        cpReqInit(remote_node, req);
        cpPrePostRecv(buffer, msginfo->datalen, req);
        cpCopyLocalDataMDesc(req, &msginfo->tag.response_mdesc);
        cpReqSend(msginfo, length, req);
}
        


static void
armci_onesided_oper(void* buffer, request_header_t *msginfo, int remote_node, cos_request_t *req)
{
        size_t length = sizeof(request_header_t);

        cpReqInit(remote_node, req);
        cpPrePostRecv(buffer, length, req);
        cpCopyLocalDataMDesc(req, &msginfo->tag.response_mdesc);
        cpReqSend(msginfo, length, req);
}



static void
armci_onesided_rmw(void *buffer, request_header_t *msginfo, int remote_node, cos_request_t *req)
{
        size_t length = sizeof(request_header_t) + msginfo->dscrlen + msginfo->datalen;

        cpReqInit(remote_node, req);
        cpPrePostRecv(buffer, msginfo->datalen, req);
        cpCopyLocalDataMDesc(req, &msginfo->tag.response_mdesc);
        cpReqSend(msginfo, length, req);
}

extern _buf_ackresp_t *_buf_ackresp_first,*_buf_ackresp_cur;

int
armci_send_req_msg(int proc, void *buf, int bytes, int tag)
{
        int cluster = armci_clus_id(proc);
        int serv    = armci_clus_info[cluster].master;
        char *buffer = (char *) buf;
        request_header_t *msginfo = (request_header_t *) buf;

      # ifdef ARMCI_LIMIT_REMOTE_REQUESTS_BY_NODE
        _armci_buf_ensure_one_outstanding_op_per_node(buf,cluster);
      # endif

        BUF_INFO_T *bufinfo=_armci_buf_to_bufinfo(msginfo);
        _buf_ackresp_t *ar = &bufinfo->ar;
        cos_request_t *req = &ar->req;

        if(msginfo->operation == PUT || ARMCI_ACC(msginfo->operation)) {
           armci_onesided_send(buffer, msginfo, cluster, req);
        }

        else if(msginfo->operation == GET) {
           buffer = (char *) buf; 
           buffer += sizeof(request_header_t);
           buffer += msginfo->dscrlen;
           armci_onesided_recv(buffer, msginfo, cluster, req);
        }

        else if(msginfo->operation == ACK) {
           armci_onesided_oper(buffer, msginfo, cluster, req);
        }

        else if(msginfo->operation == ARMCI_SWAP || msginfo->operation == ARMCI_SWAP_LONG ||
                msginfo->operation == ARMCI_FETCH_AND_ADD || 
                msginfo->operation == ARMCI_FETCH_AND_ADD_LONG) {
           buffer = (char *) buf;
           buffer += sizeof(request_header_t);
           buffer += msginfo->dscrlen; 
           armci_onesided_rmw(buffer, msginfo, cluster, req);
        }

        else {
           cosError("armci_send_req_msg: operation not supported",msginfo->operation);
        }


     // this had to be included in the portals version or shit would go down ... not sure y!
     // for now, we'll leave it in and see what happens later when we remote it
      # if 1
        ar->val = ar->valc = 0;
        if(ar==_buf_ackresp_first)_buf_ackresp_first=ar->next;
        if(ar->next!=NULL){
          ar->next->previous=ar->previous;
        }
        if(ar->previous!=NULL){
          ar->previous->next=ar->next;
          if(_buf_ackresp_cur==ar)_buf_ackresp_cur=ar->previous;
        }
        if(_buf_ackresp_cur==ar)_buf_ackresp_cur=NULL;
        ar->previous=ar->next=NULL;
      # endif

        return 0;
}



char *
armci_ReadFromDirect(int proc, request_header_t *msginfo, int len)
{
     // this is a CP funciton
        BUF_INFO_T *bufinfo = _armci_buf_to_bufinfo(msginfo);
        cos_request_t *req = &bufinfo->ar.req;
        cpReqWait(req);
        
     // return pointer to data
        char *ret = (char *) msginfo;
        ret += sizeof(request_header_t);
        ret += msginfo->dscrlen;
        return ret;
}



void
armci_WriteToDirect(int proc, request_header_t *msginfo, void *buf)
{
     // this is a DS function
        cos_desc_t resp_desc;
        cos_mdesc_t *resp_mdesc = &msginfo->tag.response_mdesc; 
        dsDescInit(resp_mdesc, &resp_desc);
        resp_desc.event_type = EVENT_LOCAL | EVENT_REMOTE;
        cosPut(buf, msginfo->datalen, &resp_desc);
        dsDescWait(&resp_desc);
}



int armci_onesided_ds_handler(void *buffer)
{
        request_header_t *request = (request_header_t *) buffer;
        size_t length = sizeof(request_header_t) + request->dscrlen + request->datalen;
        if(request->operation == PUT || ARMCI_ACC(request->operation)) {
           if(length > ARMCI_MAX_REQUEST_SIZE) {
              char *get_buffer = (char *) MessageRcvBuffer;
              cos_desc_t get_desc;
              cos_mdesc_t *mdesc = &request->tag.response_mdesc;
              dsDescInit(mdesc, &get_desc);
          //  printf("%d: get remote data not tested\n",armci_me);
          //  abort();
              get_desc.event_type = EVENT_LOCAL;
              cosGet(get_buffer, length, &get_desc);
          //  dsGetRemoteData(get_buffer, length, &get_desc);
              dsDescWait(&get_desc);
              buffer = (void *) get_buffer;
           }
        }

        if(request->operation == 0) {
           printf("%d [ds] possible zeroed buffer problem\n",armci_me);
           abort();
        }

        armci_data_server(buffer);
}



void 
armci_rcv_req(void *mesg,void *phdr,void *pdescr,void *pdata,int *buflen)
{
int i,na;
char *a;
double *tmp;

    request_header_t *msginfo = (request_header_t *)mesg;

    ARMCI_PR_SDBG("enter",msginfo->operation);
    *(void **) phdr = msginfo;

    if(0) {     
        printf("%d [ds]: got %d req (hdrlen=%d dscrlen=%d datalen=%d %d) from %d\n",
               armci_me, msginfo->operation, sizeof(request_header_t), msginfo->dscrlen,
               msginfo->datalen, msginfo->bytes,msginfo->from);
               fflush(stdout);
    }
    /* we leave room for msginfo on the client side */
    *buflen = MSG_BUFLEN - sizeof(request_header_t);
      
    // printf("%d [ds] oper=%d; bytes=%d\n",armci_me,msginfo->operation,msginfo->bytes);
    if(msginfo->bytes) {
       *(void **) pdescr = msginfo+1;
       *(void **) pdata  = msginfo->dscrlen + (char*)(msginfo+1);
          
       if(msginfo->operation == GET) {
          // the descriptor will exists after the request header
          // but there will be no data buffer
          // use the MessageRcvBuffer
          *(void**) pdata = MessageSndBuffer;
//        printf("%s (server) overriding pdata in rcv_req\n",Portals_ID());
       }     
    }
    else {
    // printf("%d [ds]: hit this\n",armci_me);
       *(void**) pdescr = NULL;
       *(void**) pdata = MessageRcvBuffer;
    }
    ARMCI_PR_SDBG("exit",msginfo->operation);
}



void
armci_server_send_ack(request_header_t *msginfo)
{
     // this is a DS function
        cos_desc_t resp_desc;
        cos_mdesc_t *resp_mdesc = &msginfo->tag.response_mdesc;
        dsDescInit(resp_mdesc, &resp_desc);
        resp_desc.event_type = EVENT_LOCAL | EVENT_REMOTE;
        cosPut(NULL, 0, &resp_desc);
        dsDescWait(&resp_desc);
}



void
x_buf_wait_ack(request_header_t *msginfo, BUF_INFO_T *bufinfo)
{
        armci_die("x_buf_wait_ack not implemented",911);
}



void
x_net_send_ack(request_header_t *msginfo, int proc, void *dst, void *src)
{
        armci_die("x_net_send_ack not implemented",911);
}



long 
x_net_offset(char *buf, int proc)
{
        armci_die("x_net_offset not implemented",911);
      # if 0
        ARMCI_PR_DBG("enter",_rem_meminfo[proc].reg_count);
        if(DEBUG_COMM) { 
           printf("\n%d:%s:buf=%p",armci_me,__FUNCTION__,buf);fflush(stdout); 
        }
        for(i=0;i<_rem_meminfo[proc].reg_count;i++) {
            if(IN_REGION(buf,_rem_meminfo[proc].reginfo[i])) {
               return((long)((char *)_rem_meminfo[proc].reginfo[i].serv_ptr-(char *)_rem_meminfo[proc].reginfo[i].base_ptr));
            }
        }
        ARMCI_PR_DBG("exit",0);
      # endif
        return 0;
}
