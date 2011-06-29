#if HAVE_CONFIG_H
#   include "config.h"
#endif

/*
 * module: global.msg.c
 * author: Jarek Nieplocha
 * date:  Wed Sep 27 09:40:10 PDT 1995
 * description: internal GA message-passing communication routines 
                implemented as wrappers to MPI/NX/MPL/TCGMSG libraries
 * notes:       non-reentrant MPICH code must be avoided in the context
 *              of interrupt-driven communication (in GA); 
 *              MPICH might be also invoked through TCGMSG interface therefore
 *              for that reason need to avoid TCGMSG on Intel and SP machines 
 *
 */

#define DEBUG  0
#define DEBUG0 0 
#define DEBUG1 0 
#define SYNC   1

#if defined SP1 || defined(SP)
#  include <mpproto.h>
#elif defined(NX)
#  if defined(PARAGON)
#     include <nx.h>
#  elif defined(DELTA)
#     include <mesh.h>
#  else
#     include <cube.h>
#  endif
#endif


#include "global.h"
#include "globalp.h"
#if HAVE_STDIO_H
#   include <stdio.h> 
#endif

#if defined(LAPI)
   /* we use MPI as native msg-passing library with lapi */
#  if !(defined(MPI) || defined(TCGMSG))
#    define MPI y
#    include <mpi.h>
#  endif
#  include <lapi.h>
#endif


#include "message.h"

#ifdef SOCKCONNECT
       typedef struct{
               int type;
               int from;
               int to;
               int len;
       }server_header_t; 
       int ga_msg_from_server=0;
       server_header_t msg_header;
       int got_header=0;
#endif

      

/*\ wrapper to PROBE operation
\*/
Integer ga_msg_probe(type, from)
     Integer type, from;
{
     if(DEBUG){
         printf("%s:%d> probing for message type=%d from=%d\n",
              GA_clus_info[GA_clus_id].hostname, ga_msg_nodeid_(), type, from);
         fflush(stdout); 
     }

#ifdef SOCKCONNECT

     /* message on the socket has priority i.e., probe returns 0 until
      * called with type argument matching type of the message available
      * from ther server socket !!
      */

     if(from != -1)gai_error("ga_msg_probe: only from=-1 works now",from);

     /* check if msg header was read before */
     if(got_header){

         if(msg_header.type == type) return 1; /* msg type available */ 
         else return(0);

     /*check if there is a message available */
     }else if(poll_server()){ 

           int msglen;
    
           /* read the message header */
           recv_from_server(&msg_header,&msglen);
           if(msglen != sizeof(msg_header))
                        gai_error("ga_msg_probe: error in header",msglen); 
           got_header=1;
           
           if(DEBUG){
              printf("%s:%d> server probing for message type=%d,%d available\n",
              GA_clus_info[GA_clus_id].hostname, ga_nodeid_(), type, 
              msg_header.type);
              fflush(stdout);
           }

           if(msg_header.type == type) return 1; /* msg type available */
           else return(0);
     }
#endif

     /* Now check the "regular" (i.e., not from server socket) messages */

#    if defined(NX)
        if (iprobe(-1L))if(infotype()==type)return (1);
        return (0);
#    elif defined(SP1) || defined(SP)
     {
       int node, rc, ttype = type, nbytes;

       node =  (from < 0) ? DONTCARE : from;
       rc = mpc_probe(&node, &ttype, &nbytes);
       if(rc <0 ) gai_error("ga_msg_probe: failed ", type);
       return (nbytes==-1 ? 0 : 1);
     }
#    elif defined(MPI)
     {
       int flag, node, ierr ;
       MPI_Status status;

       node =  (from < 0) ? MPI_ANY_SOURCE : from;
       ierr   = MPI_Iprobe(node, (int)type, MPI_COMM_WORLD, &flag, &status);
       if(ierr != MPI_SUCCESS) gai_error("ga_msg_probe: failed ", type);
       return (flag == 0 ? 0 : 1);
     }
#    else
     {
       return tcg_probe(type, from);
     }
#    endif
}
           


/*\ wrapper to a BLOCKING MESSAGE SEND operation
\*/
void ga_msg_snd(type, buffer, bytes, to)
     Integer type, bytes, to;
     void    *buffer;
{
     if(DEBUG1){
         printf("%s:%d> sending message type=%d len=%d to=%d\n",
              GA_clus_info[GA_clus_id].hostname,ga_msg_nodeid_(),type,bytes,to);
         fflush(stdout); 
     }

#ifdef SOCKCONNECT
     if(to > cluster_server || to < cluster_master){

        /* message goes outside cluster */
        server_header_t send_header; 

        if(cluster_server != ga_msg_nodeid_()){
              printf("%s:%d> ERROR sending to server type=%d len=%d to=%d\n",
              GA_clus_info[GA_clus_id].hostname,ga_msg_nodeid_(),type,bytes,to);
                gai_error("ga_msg_snd:I cannot send message outside cluster",to);
        }
        
        send_header.type = (int)type; 
        send_header.len  = (int)bytes; 
        send_header.to   = (int)to; 
        send_header.from = (int)ga_msg_nodeid_();
        
     if(DEBUG){
         printf("%s:%d> SENDING message to server type=%d len=%d to=%d\n",
              GA_clus_info[GA_clus_id].hostname,ga_msg_nodeid_(),type,bytes,to);
         fflush(stdout);
     }

        send_to_server(&send_header, sizeof(send_header));
        send_to_server(buffer, (int)bytes);

     if(DEBUG){
         printf("%s:%d> SENT message to server type=%d len=%d to=%d\n",
              GA_clus_info[GA_clus_id].hostname,ga_msg_nodeid_(),type,bytes,to);
         fflush(stdout); 
     }
        return;

     } else {
       
        /* message goes to local node ==> adjust "to" nodeid for local send */
        to -= cluster_master;
     }
#    endif

#    if defined(NX) 

        csend(type, buffer, bytes, to, 0);

#    elif defined(SP1) || defined(SP)
     {
        /* need to avoid blocking calls that disable interrupts */
        int status, msgid;

        status = mpc_send(buffer, bytes, to, type, &msgid);
        if(status == -1) gai_error("ga_msg_snd: error sending ", type);
        while((status=mpc_status(msgid)) == -1); /* nonblocking probe */
        if(status < -1) gai_error("ga_msg_snd: invalid message ID ", msgid );
     }
#    elif defined(MPI)
     {
        int ierr;
        ierr = MPI_Send(buffer, (int)bytes, MPI_CHAR, (int)to, (int)type,
                        MPI_COMM_WORLD);
        if(ierr != MPI_SUCCESS) gai_error("ga_msg_snd: failed ", type);
     }
#    else
     {
        Integer sync=SYNC;
        tcg_snd(type, buffer, bytes, to, sync);
     }
#    endif
}



/*\ wrapper to a BLOCKING MESSAGE RECEIVE operation
\*/
void ga_msg_rcv(type, buffer, buflen, msglen, from, whofrom)
     Integer type, buflen, *msglen, from, *whofrom;
     void    *buffer;
{
     if(DEBUG){
         printf("%s:%d> receiving message type=%d buflen=%d from=%d\n",
           GA_clus_info[GA_clus_id].hostname,ga_msg_nodeid_(),type,buflen,from);
         fflush(stdout);
     }

#ifdef SOCKCONNECT
     if(ga_msg_nodeid_() == cluster_server){
       
/*       if(from != -1)gai_error("ga_msg_rcv: server must use src =-1",from);*/

       if(got_header){
          if(msg_header.type != type)
              gai_error("ga_msg_rcv: server: wrong type",msg_header.type);
          if(msg_header.len > buflen) 
              gai_error("ga_msg_rcv:overflowing buffer",msg_header.len);

          /* get the message body from server socket */
          recv_from_server(buffer, msglen);
          if(*msglen  != msg_header.len)
              gai_error("ga_msg_rcv: inconsistent length header",*msglen);
          *whofrom = msg_header.from;
          if(*whofrom <0 || *whofrom >GA_n_proc)
              gai_error("ga_msg_rcv: wrong sender entry in header",*whofrom);

          got_header = 0;

     if(DEBUG){
         printf("%s:%d> Received messagefrom Server type=%d len=%d from=%d\n",
           GA_clus_info[GA_clus_id].hostname,ga_msg_nodeid_(),type,*msglen,
           *whofrom);
         fflush(stdout);
     }


          return;
       }

     }

     if(from>=0) {
             from -= cluster_master;
             if(from<0) gai_error("ga_msg_rcv: msgid problem ", from);
     }

#    endif

     /* receive the message using local message-passing library */

#    if defined(NX) 
#       ifdef  PARAGON 
        {
           long info[8], ptype=0;
           crecvx(type, buffer, buflen, from, ptype, info); 
           *msglen = info[1];
           *whofrom = info[2];
        }
#       else
           crecv(type, buffer, buflen); /* cannot receive by sender */ 
           *msglen = infocount();
           *whofrom = infonode();
           if(from!=-1 &&  *whofrom != from) {
             fprintf(stderr,"ga_msg_rcv: from %d expected %d\n",*whofrom,from);
             gai_error("ga_msg_rcv: error receiving",from);
           }
#       endif

#    elif defined(SP1) || defined(SP)
     {
        /* need to avoid blocking calls that disable interrupts */
        int status, msgid, ffrom, ttype=type; 
 
        ffrom = (from == -1)? DONTCARE: from;
        status = mpc_recv(buffer, buflen, &ffrom, &ttype, &msgid);
        if(status == -1) gai_error("ga_msg_rcv: error receiving", type);

        while((status=mpc_status(msgid)) == -1); /* nonblocking probe */
        if(status < -1) gai_error("ga_msg_rcv: invalid message ID ", msgid );
        *msglen = status;
        *whofrom = (Integer)ffrom;
     }
#    elif defined(MPI)
     {
        int ierr, count, ffrom;
        MPI_Status status;

        ffrom = (from == -1)? MPI_ANY_SOURCE : (int)from;
        ierr = MPI_Recv(buffer, (int)buflen, MPI_CHAR, ffrom, (int)type,
               MPI_COMM_WORLD, &status);
        if(ierr != MPI_SUCCESS) gai_error("ga_msg_rcv: Recv failed ", type);

        ierr = MPI_Get_count(&status, MPI_CHAR, &count);
        if(ierr != MPI_SUCCESS) gai_error("ga_msg_rcv: Get_count failed ", type);
        *whofrom = (Integer)status.MPI_SOURCE;
        *msglen  = (Integer)count;
     }
#    else
     {
        Integer sync=SYNC;
        tcg_rcv(type, buffer, buflen, msglen, from, whofrom, sync);
     }
#    endif

#ifdef SOCKCONNECT
     /* adjust whofrom to reflect cluster environment */ 
     if(*whofrom < cluster_master) *whofrom += cluster_master;
#endif
     if(DEBUG0){
         printf("%s:%d> received message from local type=%d len=%d from=%d\n",
           GA_clus_info[GA_clus_id].hostname,ga_msg_nodeid_(),type,*msglen,
           *whofrom);
         fflush(stdout);
     }

}



/*\ wrapper to a NONBLOCKING MESSAGE RECEIVE
\*/
msgid_t ga_msg_ircv(type, buffer, buflen, from)
     Integer type, buflen, from;
     void    *buffer;
{
msgid_t msgid;

     if(DEBUG){
         printf("%s:%d> async receiving message type=%d buflen=%d from=%d\n",
           GA_clus_info[GA_clus_id].hostname,ga_msg_nodeid_(),type,buflen,from);
         fflush(stdout);
     }

#ifdef SOCKCONNECT
     if(ga_msg_nodeid_() == cluster_server){
        gai_error("ga_msg_ircv: server cannot use irecv",type);
     }
     if(from>=0) {
             from -= cluster_master;
             if(from<0) gai_error("ga_msg_ircv: msgid problem ", from);
     }
#endif

#    if defined(NX) 

#       ifdef  PARAGON
        {
           long ptype=0;
           /*msginfo is NX internal */
           msgid = irecvx(type, buffer, buflen, from, ptype, msginfo);
        }
#       else
           msgid = irecv(type, buffer, buflen); /* cannot receive by sender */
#       endif

#    elif defined(SP1) || defined(SP)
     {
        int status;
        static int ffrom, ttype; /*  MPL writes upon message arrival */
        ttype = type;
        ffrom = (from == -1)? DONTCARE: from;
        status = mpc_recv(buffer, buflen, &ffrom, &ttype, &msgid);
        if(status == -1) gai_error("ga_msg_ircv: error receiving", type);
     }
     /*****  we use MPI with lapi ********/
#    elif defined(MPI)
     {
        int ierr, count, ffrom;
        ffrom = (from == -1)? MPI_ANY_SOURCE : (int)from;
        ierr = MPI_Irecv(buffer, (int)buflen, MPI_CHAR, ffrom, (int)type,
               MPI_COMM_WORLD, &msgid);
        if(ierr != MPI_SUCCESS) gai_error("ga_msg_ircv: Recv failed ", type);
     }
#    else
     {
       Integer sync=ASYNC, msglen, whofrom;
       tcg_rcv(type, buffer, buflen, &msglen, from, &whofrom, sync);
       msgid = from; /*TCGMSG waits for all comms to/from node */
     }
#    endif

     return(msgid);
}



/*\ wrapper to BLOCKING MESSAGE WAIT operation  
 *  Note: values returned in whofrom and msglen might not be reliable (updated)
\*/
void ga_msg_wait(msgid, whofrom, msglen)
msgid_t msgid;
Integer *whofrom, *msglen;
{
     if(DEBUG){
         printf("%s:%d> WAITING FOR MESSAGE\n",
              GA_clus_info[GA_clus_id].hostname,ga_msg_nodeid_());
         fflush(stdout);
     }

#    if defined(NX) 

        msgwait(msgid);
/*        *msglen = infocount();*/
/*        *whofrom = infonode();*/

#    elif defined(SP1) || defined(SP)
     {
        int status;
        while((status=mpc_status(msgid)) == -1); /* nonblocking probe */
        if(status < -1) gai_error("ga_wait_msg: invalid message ID ", msgid);
        *msglen = status;
        /* whofrom is currently not retrieved from MPL */
     }
#    elif defined(MPI)
     {
        int ierr, count;
        MPI_Status status;

        ierr = MPI_Wait(&msgid, &status);
        if(ierr != MPI_SUCCESS)gai_error("ga_msg_wait: failed ", 1);
        ierr = MPI_Get_count(&status, MPI_CHAR, &count);
        if(ierr != MPI_SUCCESS) gai_error("ga_msg_wait: Get_count failed",2);
        *whofrom = (Integer)status.MPI_SOURCE;
        *msglen  = (Integer)count;
     }
#    else
        tcg_waitcom(msgid); /* cannot get whofrom and msglen from TCGMSG */
#    endif
} 


/*\ total NUMBER OF PROCESSES that can communicate using message-passing
 *  Note: might be larger than the value returned by ga_NNODES_()
\*/
Integer ga_msg_nnodes_()
{
#  ifdef SOCKCONNECT
     return GA_n_proc;
#  endif
#  ifdef MPI
     int numprocs;
     MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
     return((Integer)numprocs);
#  else
     return (tcg_nnodes());
#  endif
}


/*\ message-passing RANK/NUMBER of current PROCESS
 *  Note: might be different than the value returned by ga_NODEID_()
\*/
Integer ga_msg_nodeid_()
{
Integer msg_id;
#  ifdef MPI
     int myid;

     MPI_Comm_rank(MPI_COMM_WORLD,&myid);
     msg_id = ((Integer)myid);
#  else
     msg_id =  (tcg_nodeid());
#  endif
#  ifdef SOCKCONNECT
     msg_id += cluster_master;
     if(msg_id >= GA_n_proc) gai_error("ga_msg_nodeid:what is going on?",msg_id);
#  endif
   return (msg_id);
}


void ga_msg_brdcst(type, buffer, len, root)
Integer type, len, root;
void*   buffer; 
{
#  ifdef MPI
      MPI_Bcast(buffer, (int)len, MPI_CHAR, (int)root, MPI_COMM_WORLD);
#  else
      tcg_brdcst(type, buffer, len, root);
#  endif
}



#if defined(SP1) || defined(SP) || defined(LAPI)

/* This paranoia is required to assure that there is always posted receive 
 * for synchronization message. MPL (and EUIH) "in order message delivery"
 * rule causes that synchronization message arriving from another node
 * must be received before any following request (rcvncall) shows up.
 * 
 * MPL synchronization must be avoided as it is not compatible with rcvncall
 */

Integer syn_me, syn_up, syn_left, syn_right, syn_root=0;
static msgid_t syn_up_id, syn_left_id, syn_right_id;
Integer syn_type1=37772, syn_type2=37773, syn_type3=37774;
Integer first_time=1, xsyn;

void sp_init_sync()
{
  if (syn_me != syn_root) syn_up_id = ga_msg_ircv(syn_type1, &xsyn, 1, syn_up);
  if (syn_left > -1 ) syn_left_id = ga_msg_ircv(syn_type2, &xsyn, 1, syn_left);
  if (syn_right > -1) syn_right_id = ga_msg_ircv(syn_type2, &xsyn, 1,syn_right);
}


void sp_sync()
{
Integer len, from, xsyn;
  /* from root down */
  if (syn_me != syn_root){
      ga_msg_wait(syn_up_id, &from, &len );
      syn_up_id = ga_msg_ircv(syn_type1, &xsyn, 0, syn_up);
  }
  if (syn_left > -1)  ga_msg_snd(syn_type1, &xsyn, 0, syn_left);
  if (syn_right > -1) ga_msg_snd(syn_type1, &xsyn, 0, syn_right);

  /* up to root */
  if (syn_left > -1 ){
      ga_msg_wait(syn_left_id, &from, &len );
      syn_left_id = ga_msg_ircv(syn_type2, &xsyn, 0, syn_left);
  }
  if (syn_right > -1){
      ga_msg_wait(syn_right_id, &from, &len );
      syn_right_id = ga_msg_ircv(syn_type2, &xsyn, 0, syn_right);
  }
  if (syn_me != syn_root) ga_msg_snd(syn_type2, &xsyn, 0, syn_up);

}
#endif

static long sync_cnt=0;

/*\ Synchronization using message-passing
 *  Note: ga_sync might not be calling it at all on some platforms
\*/
void ga_msg_sync_()
{
   if(DEBUG){
       printf("%s:%d> ga_msg_sync\n",
              GA_clus_info[GA_clus_id].hostname,ga_msg_nodeid_());
       fflush(stdout);
   }

#  ifdef LAPI
   {
     extern lapi_handle_t lapi_handle;
     if(LAPI_Fence(lapi_handle)) gai_error("lapi_gfence failed",0);
   }
#  endif

 
#  if defined(SP1) || defined(SP)
   {
      int i_on;
      /* on SP sync needs extra care to avoid conflict with rcvncall */
      Integer group_participate();
      if(first_time){
#       ifdef IWAY
            group_participate(&syn_me, &syn_root, &syn_up, &syn_left,
                              &syn_right, CLUST_GRP);
#       else
            group_participate(&syn_me, &syn_root, &syn_up, &syn_left,
                              &syn_right, ALL_GRP);
#       endif
        sp_init_sync();
        first_time =0;
      }

#     if (defined(SP) || defined(SP1)) && !defined(AIX3)
              i_on = mpc_queryintr();
              mpc_disableintr();
#     endif
      /* one sync should be enough but it is not -- this code needs more work!*/
      sp_sync();
      sp_sync();
#     if (defined(SP) || defined(SP1)) && !defined(AIX3)
              if(i_on) mpc_enableintr();
#     endif
   }
#  elif defined IWAY
   {
      char sum='+';
      Integer dummy=1; 
      ga_igop_clust(GA_TYPE_GOP, &dummy, 1, &sum, CLUST_GRP);
   }
#  elif defined(MPI)
      MPI_Barrier(MPI_COMM_WORLD);
#  else
   {
      Integer type = GA_TYPE_SYN;
      tcg_synch(type);
   }
#  endif
   if(DEBUG0){
       printf("%s:%d> ga_msg_sync completed %ld\n",
              GA_clus_info[GA_clus_id].hostname,ga_msg_nodeid_(),sync_cnt++);
       fflush(stdout);
   }

}


/* Note: collective comms  below could be implemented as trivial wrappers
 * to MPI if it was reentrant
 */


/*\ determines if calling process participates in collective comm
 *  also, parameters for a given communicator are found
\*/
Integer group_participate(me, root, up, left, right, group)
     Integer *me, *root, *up, *left, *right, group;
{
     Integer nproc, index;

     switch(group){
                           /* all message-passing processes */
           case  ALL_GRP:  *root = 0;
                           *me = ga_msg_nodeid_(); nproc = ga_msg_nnodes_();
                           *up = (*me-1)/2;       if(*up >= nproc) *up = -1;
                           *left  =  2* *me + 1;  if(*left >= nproc) *left = -1;
                           *right =  2* *me + 2;  if(*right >= nproc)*right =-1;

                           break;
                           /* all GA (compute) processes in cluster */
          case CLUST_GRP:  *root = cluster_master;
                           *me = ga_msg_nodeid_(); nproc =cluster_compute_nodes;
                           if(*me < *root || *me >= *root +nproc)
                                                return 0; /*does not*/ 

                           index  = *me  - *root;
                           *up    = (index-1)/2 + *root;  
                           *left  = 2*index + 1 + *root; 
                                    if(*left >= *root+nproc) *left = -1;
                           *right = 2*index + 2 + *root; 
                                    if(*right >= *root+nproc) *right = -1;

                           break;
                           /* all msg-passing processes in cluster */
      case ALL_CLUST_GRP:  *root = cluster_master;
                           *me = ga_msg_nodeid_(); 
                           nproc = cluster_nodes; /* +server*/
                           if(*me < *root || *me >= *root +nproc)
                                                return 0; /*does not*/

                           index  = *me  - *root;
                           *up    = (index-1)/2 + *root;   
                                    if( *up < *root) *up = -1;
                           *left  = 2*index + 1 + *root;
                                    if(*left >= *root+nproc) *left = -1;
                           *right = 2*index + 2 + *root;
                                    if(*right >= *root+nproc) *right = -1;

                           break;
                           /* cluster masters (designated process in cluster) */
    case INTER_CLUST_GRP:  *root = GA_clus_info[0].masterid;
                           *me = ga_msg_nodeid_();  nproc = GA_n_clus;

                           if(*me != cluster_master) return 0; /*does not*/

                           *up    = (GA_clus_id-1)/2;
                           if(*up >= nproc) *up = -1;
                             else *up = GA_clus_info[*up].masterid;

                           *left  = 2*GA_clus_id+ 1;
                           if(*left >= nproc) *left = -1;
                             else *left = GA_clus_info[*left].masterid;

                           *right = 2*GA_clus_id+ 2;
                           if(*right >= nproc) *right = -1;
                             else *right = GA_clus_info[*right].masterid;

#                          ifdef IWAY 
                             /* WARNING: will break if more than 2 clusters !!*/
                             if(GA_n_clus>2)
                                gai_error("group_participate:fix me too",0);
                             if(*up>-1)    *up = cluster_server; 
                             if(*left>-1)  *left = cluster_server; 
                             if(*right>-1) *right = cluster_server; 
#                          endif

                           break;
                 default:  gai_error("group_participate: wrong group ", group);
     }
     return (1);
}




/*\ BROADCAST 
 *  internal GA routine that is used in data server mode
 *  with predefined communicators
\*/
void ga_brdcst_clust(type, buf, len, originator, group)
     Integer type, len, originator, group;
     void *buf;
{
     Integer me, lenmes, from, root=0;
     Integer up, left, right, participate;

#    ifdef SOCKCONNECT
       /* data server recognizes fixed set of message types */
       type = GA_TYPE_BRD;
#    endif

     participate = group_participate(&me, &root, &up, &left, &right, group);

     /*  cannot exit just yet -->  send the data to root */

     if (originator != root ){
       if(me == originator) ga_msg_snd(type, buf, len, root); 
       if(me == root) ga_msg_rcv(type, buf, len, &lenmes, originator, &from); 
     }

     if( ! participate) return;

     if (me != root) ga_msg_rcv(type, buf, len, &lenmes, up, &from);
     if (left > -1)  ga_msg_snd(type, buf, len, left);
     if (right > -1) ga_msg_snd(type, buf, len, right);
}



/*\ BROADCAST
\*/
void FATR ga_brdcst_(type, buf, len, originator)
     Integer *type, *len, *originator;
     void *buf;
{
     Integer orig_clust, tcg_orig_node, tcg_orig_master; 

#ifdef USE_VAMPIR
     vampir_begin(GA_BRDCST,__FILE__,__LINE__);
#endif
     if(DEBUG1){
         printf("%s:%d> broadcast type=%d len=%d root=%d\n",
              GA_clus_info[GA_clus_id].hostname,ga_msg_nodeid_(),*type,*len, *originator);
         fflush(stdout);
     }

     if(ClusterMode){
#       ifdef IWAY
           ga_sync_();
#       endif
        /* originator is GA node --> need to transform it into msg nodeid */
        orig_clust = ClusterID(*originator); 
        tcg_orig_master =  GA_clus_info[orig_clust].masterid;
        tcg_orig_node   =  *originator + orig_clust;
        if(orig_clust == GA_clus_id){
           ga_brdcst_clust(*type, buf, *len, tcg_orig_node, CLUST_GRP);
           ga_brdcst_clust(*type, buf, *len, tcg_orig_master, INTER_CLUST_GRP);
        }else{
           ga_brdcst_clust(*type, buf, *len, tcg_orig_master, INTER_CLUST_GRP);
           ga_brdcst_clust(*type, buf, *len, cluster_master, CLUST_GRP);
        }
#       ifdef IWAY
           ga_sync_();
#       endif
     } else {
        /* use TCGMSG as a wrapper to native implementation of broadcast */
        Integer gtype,gfrom,glen;
        gtype =(long) *type; gfrom =(long) *originator; glen =(long) *len;
#       if defined(SP1)|| defined(SP)
            ga_sync_();
            /*            brdcst_(&gtype,buf,&glen,&gfrom);*/
            ga_msg_brdcst(gtype, buf, glen, gfrom);
            ga_sync_();
#       else
            ga_msg_brdcst(gtype, buf, glen, gfrom);
#       endif
     }
#ifdef USE_VAMPIR
     vampir_end(GA_BRDCST,__FILE__,__LINE__);
#endif
}


/*\ reduce operation for double
\*/
static void ddoop(n, op, x, work)
     long n;
     char *op;
     double *x, *work;
{
  if (strncmp(op,"+",1) == 0)
    while(n--)
      *x++ += *work++;
  else if (strncmp(op,"*",1) == 0)
    while(n--)
      *x++ *= *work++;
  else if (strncmp(op,"max",3) == 0)
    while(n--) {
      *x = GA_MAX(*x, *work);
      x++; work++;
    }
  else if (strncmp(op,"min",3) == 0)
    while(n--) {
      *x = GA_MIN(*x, *work);
      x++; work++;
    }
  else if (strncmp(op,"absmax",6) == 0)
    while(n--) {
      register double x1 = GA_ABS(*x), x2 = GA_ABS(*work);
      *x = GA_MAX(x1, x2);
      x++; work++;
    }
  else if (strncmp(op,"absmin",6) == 0)
    while(n--) {
      register double x1 = GA_ABS(*x), x2 = GA_ABS(*work);
      *x = GA_MIN(x1, x2);
      x++; work++;
    }
  else
    gai_error("ga_ddoop: unknown operation requested", (long) n);
}



/*\ reduce operation for integer
\*/
static void idoop(n, op, x, work)
     long n;
     char *op;
     Integer *x, *work;
{
  if (strncmp(op,"+",1) == 0)
    while(n--)
      *x++ += *work++;
  else if (strncmp(op,"*",1) == 0)
    while(n--)
      *x++ *= *work++;
  else if (strncmp(op,"max",3) == 0)
    while(n--) {
      *x = GA_MAX(*x, *work);
      x++; work++;
    }
  else if (strncmp(op,"min",3) == 0)
    while(n--) {
      *x = GA_MIN(*x, *work);
      x++; work++;
    }
  else if (strncmp(op,"absmax",6) == 0)
    while(n--) {
      register Integer x1 = GA_ABS(*x), x2 = GA_ABS(*work);
      *x = GA_MAX(x1, x2);
      x++; work++;
    }
  else if (strncmp(op,"absmin",6) == 0)
    while(n--) {
      register Integer x1 = GA_ABS(*x), x2 = GA_ABS(*work);
      *x = GA_MIN(x1, x2);
      x++; work++;
    }
  else if (strncmp(op,"or",2) == 0) 
    while(n--) {
      *x |= *work;
      x++; work++;
    }
  else
    gai_error("ga_idoop: unknown operation requested", (long) n);
}



#define BUF_SIZE 10000
DoublePrecision _gops_work[BUF_SIZE];

/*\  global operations:
 *     . all processors participate  
 *     . all processors in the cluster participate  
 *     . master processors in each cluster participate  
\*/
void ga_dgop_clust(type, x, n, op, group)
     Integer type, n, group;
     DoublePrecision *x;
     char *op;
{
     Integer  me, lenmes, from, len, root;
     DoublePrecision *work = _gops_work, *origx = x;
     Integer ndo, up, left, right, orign = n;

#    ifdef IWAY
       /* data server recognizes fixed set of message types */
       type = GA_TYPE_GOP;
#    endif


     if( ! group_participate(&me, &root, &up, &left, &right, group)) return;

     while ((ndo = (n<=BUF_SIZE) ? n : BUF_SIZE)) {
	 len = lenmes = ndo*sizeof(DoublePrecision);

         if (left > -1) {
           ga_msg_rcv(type, (char *) work, len, &lenmes, left, &from);
           ddoop(ndo, op, x, work);
         }
         if (right > -1) {
           ga_msg_rcv(type, (char *) work, len, &lenmes, right, &from);
           ddoop(ndo, op, x, work);
         }
         if (me != root) ga_msg_snd(type, x, len, up); 

       n -=ndo;
       x +=ndo;
     }
     /* Now, root broadcasts the result down the binary tree */
     len = orign*sizeof(DoublePrecision);
     ga_brdcst_clust(type, (char *) origx, len, root, group);
}


#ifdef TIME_DGOP
double t0_dgop, t_dgop=0., n_dgop=0., s_dgop=0., tcg_time();
#endif
/*\ GLOBAL OPERATIONS
 *  (C)
 *  We cannot use TCGMSG in data-server mode
 *  where only compute processes participate
\*/
void gai_dgop(Integer type, DoublePrecision *x, Integer n, char *op)
{

#ifdef USE_VAMPIR
    vampir_begin(GA_DGOP,__FILE__,__LINE__);
#endif
#ifdef TIME_DGOP
     t0_dgop = tcg_time();
#endif
     if(ClusterMode){
#       ifdef IWAY
           ga_sync_();
#       endif
        ga_dgop_clust(type, x, n, op, CLUST_GRP);
        ga_dgop_clust(type, x, n, op, INTER_CLUST_GRP);
        ga_brdcst_clust(type, x, n*sizeof(DoublePrecision), cluster_master,
                        CLUST_GRP);
#       ifdef IWAY
           ga_sync_();
#       endif
     } else {
        /* use TCGMSG as a wrapper to native implementation of global ops */
#       if defined(SP1) || defined(SP)
            ga_msg_sync_();
#       endif
#       ifdef MPI
            ga_dgop_clust(type, x, n, op, ALL_GRP);
#       else
            tcg_dgop(type, x, n, op);
#       endif
#       if defined(SP1) || defined(SP)
            ga_msg_sync_();
#       endif
     }
#ifdef TIME_DGOP
     t_dgop += tcg_time() - t0_dgop;
     n_dgop+= 1;
     s_dgop+= (double)n;
#endif
#ifdef USE_VAMPIR
    vampir_end(GA_DGOP,__FILE__,__LINE__);
#endif
}


/*\ GLOBAL OPERATIONS 
 *  Fortran
\*/
void ga_dgop_(Integer *type, DoublePrecision *x, Integer *n, char *op, int len)
{
long gtype,gn;
     gtype = (long)*type; gn = (long)*n; 

     gai_dgop(gtype, x, gn, op);
}


/*\  global operations:
 *     . all processors participate
 *     . all processors in the cluster participate
 *     . master processors in each cluster participate
\*/
void ga_igop_clust(type, x, n, op, group)
     Integer type, n, group;
     Integer *x;
     char *op;
{
     Integer  me, lenmes,  from, len, root=0 ;
     Integer *work = (Integer*)_gops_work, *origx = x;
     Integer ndo, up, left, right, orign =n;

#    ifdef IWAY
       /* data server recognizes fixed set of message types */
       type = GA_TYPE_GOP;
#    endif

     if( ! group_participate(&me, &root, &up, &left, &right, group)) return;

     while ((ndo = (n<=BUF_SIZE) ? n : BUF_SIZE)) {
	 len = lenmes = ndo*sizeof(Integer);

         if (left > -1) {
           ga_msg_rcv(type, (char *) work, len, &lenmes, left, &from);
	   idoop(ndo, op, x, work); 
         }
         if (right > -1) {
           ga_msg_rcv(type, (char *) work, len, &lenmes, right, &from);
	   idoop(ndo, op, x, work); 
         }
         if (me != root) ga_msg_snd(type, x, len, up);

       n -=ndo;
       x +=ndo;
     }
     /* Now, root broadcasts the result down the binary tree */
     len = orign*sizeof(Integer);
     ga_brdcst_clust(type, (char *) origx, len, root, group);
}


/*\ GLOBAL OPERATIONS
 *  (C)
 *  We cannot use TCGMSG in data-server mode
 *  where only compute processes participate
\*/
void gai_igop(Integer type, Integer *x, Integer n, char *op)
{

#ifdef USE_VAMPIR
     vampir_begin(GA_IGOP,__FILE__,__LINE__);
#endif
     if(ClusterMode){
#       ifdef IWAY
           ga_sync_();
#       endif
        ga_igop_clust(type, x, n, op, CLUST_GRP);
        ga_igop_clust(type, x, n, op, INTER_CLUST_GRP);
        ga_brdcst_clust(type, x, n*sizeof(Integer), cluster_master, CLUST_GRP);
#       ifdef IWAY
           ga_sync_();
#       endif
     } else {
        /* use TCGMSG as a wrapper to native implementation of global ops */
#       if defined(SP1) || defined(SP)
            ga_msg_sync_();
#       endif
#       ifdef MPI
            ga_igop_clust(type, x, n, op, ALL_GRP);
#       else
            tcg_igop(type, x, n, op);
#       endif
#       if defined(SP1) || defined(SP)
            ga_msg_sync_();
#       endif
     }
#ifdef USE_VAMPIR
     vampir_end(GA_IGOP,__FILE__,__LINE__);
#endif
}




/*\ GLOBAL OPERATIONS 
 *  Fortran
\*/
void ga_igop_(Integer *type, Integer *x, Integer *n, char *op, int len)
{
long gtype,gn;
     gtype = (long)*type; gn = (long)*n;

     gai_igop(gtype, x, gn, op);
}
