/*$Id: armci-vapi.h,v 1.21.2.1 2007-03-06 19:50:24 vinod Exp $ */
#ifndef _VAPI_H
#define _VAPI_H

/*vapi includes*/
#ifdef MELLANOX
#include <vapi.h>
#include <evapi.h>
#include <mtl_common.h>
#include <ib_defs.h>
#include <vapi_common.h>
#endif

#ifdef OPENIB
#include <infiniband/verbs.h>
#endif


#define DSCRID_NBDSCR 10000

#define MAX_RDMA_SIZE		(8388608)
#define DEFAULT_ADDR_LEN	(8) /* format length of hcalid/qp_num.*/

#define DEFAULT_PORT            (1) /*for vapi*/
#define DEFAULT_MTU             (MTU1024)
#define DEFAULT_PSN             (0)
#define DEFAULT_PKEY_IX         (0)
#define DEFAULT_P_KEY           (0x0)
#define DEFAULT_MIN_RNR_TIMER   (5)
#define DEFAULT_SERVICE_LEVEL   (0)
#define DEFAULT_TIME_OUT        (5)
#define DEFAULT_STATIC_RATE     (2)
#define DEFAULT_SRC_PATH_BITS   (0)
#define DEFAULT_RETRY_COUNT     (1)
#define DEFAULT_RNR_RETRY       (1)


#define DEFAULT_R_KEY           (0x0)
#define DEFAULT_L_KEY           (0x0)

#define  DEFAULT_MAX_WQE	2048/*(1023)*/

typedef struct {
    void *data_ptr;         /* pointer where the data should go */
    long ack;               /* header ack */
    void *ack_ptr;          /* pointer where the data should go */
#if defined(PEND_BUFS)
  unsigned int imm_msg:1;
  unsigned int data_len:31;
#endif
} msg_tag_t;

typedef struct {
#ifdef MELLANOX
        VAPI_sr_desc_t sdscr;
        VAPI_sg_lst_entry_t    ssg_entry;
        VAPI_rr_desc_t rdscr;
        VAPI_sg_lst_entry_t    rsg_entry;
#endif
#ifdef OPENIB
        struct ibv_send_wr sdscr;
        struct ibv_sge ssg_entry;
        struct ibv_recv_wr rdscr;
        struct ibv_sge rsg_entry;
#endif
} armci_vapi_field_t;

typedef struct {
#ifdef MELLANOX
        VAPI_lkey_t lkey;
        VAPI_rkey_t rkey;
        VAPI_mr_hndl_t memhndl;
#endif
#ifdef OPENIB
        uint32_t rkey;
        uint32_t lkey;
        struct ibv_mr *memhndl;
#endif
}armci_vapi_memhndl_t;

extern char * armci_vapi_client_mem_alloc(int);

typedef struct {
        int tag;
	int issg;
#ifdef MELLANOX
        VAPI_sr_desc_t sdescr;
        VAPI_rr_desc_t rdescr;
        VAPI_sg_lst_entry_t sg_entry[56]; /*ff:this has to be malloced*/
#endif
#ifdef OPENIB
        struct ibv_send_wr sdescr;
        struct ibv_recv_wr rdescr;
        struct ibv_sge sg_entry[56]; /*ff:this has to be malloced*/
#endif
        int numofsends;
        int numofrecvs;
	int myindex;
} sr_descr_t;

typedef struct {
        int tag;
	int issg;
#ifdef MELLANOX
        VAPI_rr_desc_t descr;
        VAPI_sg_lst_entry_t sg_entry[56]; /*ff:this has to be malloced*/
#endif
#ifdef OPENIB
        struct ibv_recv_wr descr;
        struct ibv_send_wr sg_entry[56]; /*ff:this has to be malloced*/
#endif
        int numofrecvs;
	    int myindex;
} rdescr_t;

#if defined(PEND_BUFS)
#ifdef OPENIB
/* typedef struct { */
/*   struct ibv_recv_wr  dscr; */
/*   struct ibv_sge      sg_entry; */
/* } IMMBUF_NW_T; */
/* typedef struct { */
/*   struct ibv_sge sg_entry; */
/*   struct ibv_recv_wr rdscr; */
/*   struct ibv_send_wr sdscr; */
/* } PENDBUF_NW_T; */
#define IMMBUF_NW_T 	    \
  struct ibv_recv_wr  dscr; \
  struct ibv_sge sg_entry;  \
  int send_pending; 
#define PENDBUF_NW_T        \
  struct ibv_sge sg_entry;  \
  struct ibv_recv_wr rdscr; \
  struct ibv_send_wr sdscr; 

#define IS_IMM_MSG(msginfo) ((msginfo).tag.imm_msg)

void armci_complete_immbuf(void *vbuf);
void armci_complete_pendbuf(void *buf);
/*Note that start_put and start_get cannot report completion from
  within. They have to just return and report completion later.  
 */
void armci_pbuf_start_put(void *src, void *dst, int bytes, int proc,
			  int bufid);
void armci_pbuf_start_get(void *msg_info,
			  void *src, void *dst, int bytes, int proc, 
			  int bufid);
#else
#error "PEND_BUFS only implemented for OPENIB"
#endif
#endif

void armci_client_nbcall_complete(sr_descr_t *,int,int);
void armci_vapi_set_mark_buf_send_complete(int);

#define ARMCI_MEMHDL_T armci_vapi_memhndl_t

#define REGIONS_REQUIRE_MEMHDL

#define PIPE_BUFSIZE__  (4096)
#define PIPE_MIN_BUFSIZE 1024 
#define PIPE_MEDIUM_BUFSIZE (2*1024)


#define VBUF_DLEN 4*64*1023
#define MSG_BUFLEN_DBL ((VBUF_DLEN)>>3)

#if defined(PEND_BUFS)
#define IMM_BUF_NUM_DEFAULT 4
#define IMM_BUF_LEN_DEFAULT 2048
#define PENDING_BUF_NUM_DEFAULT 20
#define PENDING_BUF_LEN_DEFAULT (VBUF_DLEN)
#endif

#ifdef PIPE_BUFSIZE
#  define STRIDED_GET_BUFLEN_DBL 31*1024
#  define STRIDED_GET_BUFLEN (STRIDED_GET_BUFLEN_DBL<<3)
#  define MAX_BUFLEN (STRIDED_GET_BUFLEN+EXTRA_MSG_BUFLEN)
#else
#  define MAX_BUFLEN (MSG_BUFLEN+EXTRA_MSG_BUFLEN)
#endif
#define BALANCE_BUFFERS
#ifdef BALANCE_BUFFERS
#  define BALANCE_FACTOR 1.6
/* #  define BALANCE_FACTOR 2.0 */
#  define BALANCE_BUFSIZE 25000
#endif

#define BUF_EXTRA_FIELD_T armci_vapi_field_t 
#define GET_SEND_BUFFER _armci_buf_get
#define FREE_SEND_BUFFER _armci_buf_release
#ifdef MELLANOX
#define INIT_SEND_BUF(_field,_snd,_rcv) _snd=1;_rcv=1;memset(&((_field).sdscr),0,sizeof(VAPI_sr_desc_t));(_field).sdscr.id=avail+1;armci_vapi_set_mark_buf_send_complete(avail+1)
#endif
#ifdef OPENIB
#define INIT_SEND_BUF(_field,_snd,_rcv) _snd=1;_rcv=1;memset(&((_field).sdscr),0,sizeof(struct ibv_send_wr));(_field).sdscr.wr_id=avail+1;armci_vapi_set_mark_buf_send_complete(avail+1)
#endif
#define BUF_ALLOCATE armci_vapi_client_mem_alloc

#define CLEAR_SEND_BUF_FIELD(_field,_snd,_rcv,_to,_op) armci_vapi_complete_buf((armci_vapi_field_t *)(&(_field)),(_snd),(_rcv),(_to),(_op));_snd=0;_rcv=0;_to=0

#define TEST_SEND_BUF_FIELD(_field,_snd,_rcv,_to,_op,_pret) armci_vapi_test_buf((armci_vapi_field_t *)(&(_field)),(_snd),(_rcv),(_to),(_op),(_pret))

#define CLIENT_BUF_BYPASS 1

#define _armci_bypass 1

#define COMPLETE_HANDLE _armci_buf_complete_nb_request

#if defined(ALLOW_PIN)
#  define NB_CMPL_T sr_descr_t*
#  define ARMCI_NB_WAIT(_cntr) if(_cntr)if(nb_handle->tag==(_cntr)->tag)\
          armci_client_nbcall_complete(_cntr,nb_handle->tag,nb_handle->op);
#  define CLEAR_HNDL_FIELD(_x) _x=NULL
#endif

/* #define DIRECT_PUT_MIN_SIZE 8192 */
#define DIRECT_PUT_MIN_SIZE 0

#define LONG_GET_THRESHOLD 2147483648
#define LONG_GET_THRESHOLD_STRIDED 2147483648
#ifndef ARMCI_STAMP
#error "ARMCI_STAMP used to define ARMCI_VAPI_COMPLETE not found!"
#define ARMCI_VAPI_COMPLETE 1088451863
#else
#define ARMCI_VAPI_COMPLETE ARMCI_STAMP
#endif
#define ARMCI_POST_SCATTER 1000000001
#define ARMCI_VAPI_CLEAR 0
#ifdef MELLANOX
#define VAPI_SGGET_MIN_COLUMN 720
#define VAPI_SGPUT_MIN_COLUMN 720
#endif
#ifdef OPENIB
/* #define VAPI_SGGET_MIN_COLUMN 2147483648 */
#define VAPI_SGPUT_MIN_COLUMN 2147483648
#define VAPI_SGGET_MIN_COLUMN 10
/* #define VAPI_SGPUT_MIN_COLUMN 720 */
#endif
#define DSCRID_SCATTERCLIENT 70000
#define DSCRID_SCATTERCLIENT_END 70000+9999

#define MAX_PENDING 32

#define HAS_RDMA_GET

#if defined(OPENIB)
#if 1
#define PUT_NO_SRV_COPY /*server rdma-s from to client buffers (in
			  buffers.c) to
			  remote memory (bypassing
			  servers intermediate buffers) when possible*/
#define GET_NO_SRV_COPY /*server rdma-s from remote memory to client
			  buffers (in buffers.c) (bypassing server
			  intermediate buffers) when possible*/ 
int no_srv_copy_nsegs_ulimit();/*max #contig segs for which we avoid server copy. Too many segments might overflow a queue*/
#endif
#define GET_STRIDED_COPY_PIPELINED /*client copies data from buffers
				     to user memory in segments rather
				     than waiting for all the data*/
#define GET_STRIDED_COPY_PIPELINED_SIZE (1*1024) /*size of each segment*/
#endif

#endif /* _VAPI_CONST_H */

extern void armci_wait_ack(char *buffer);
extern void armci_complete_multi_sglist_sends(int proc);
extern void armci_client_direct_send(int p,void *src_buf, void *dst_buf, int len,void** contextptr,int nbtag,ARMCI_MEMHDL_T *lochdl,ARMCI_MEMHDL_T *remhdl);
