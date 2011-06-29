#if HAVE_CONFIG_H
#   include "config.h"
#endif

#if HAVE_STDIO_H
#   include <stdio.h>
#endif

extern void qsort(void *base, size_t nmemb, size_t size, int(*compar)(const void *, const void *));

#include "srftoc.h"
#include "sndrcv.h"
#include "tcgmsgP.h"

#ifdef USE_VAMPIR
#   include "tcgmsg_vampir.h"
#endif

extern Integer MatchShmMessage();
extern void msg_wait();
extern long DEBUG_;

#define INVALID_NODE -3333      /* used to stamp completed msg in the queue */
#define MAX_Q_LEN MAX_PROC           /* Maximum no. of outstanding messages */
static  volatile Integer n_in_msg_q = 0;   /* actual no. in the message q */
static  struct msg_q_struct{
    Integer   msg_id;
    Integer   node;
    Integer   type;
} msg_q[MAX_Q_LEN];


/**
 * Return 1/0 (TRUE/FALSE) if a message of the given type is available
 * from the given node.  If the node is specified as -1, then all nodes
 * will be examined.  Some attempt is made at ensuring fairness.
 *
 * If node is specified as -1 then this value is overwritten with the
 * node that we got the message from.
 */
Integer ProbeNode(Integer *type, Integer *node)
{
    static Integer  next_node = 0;

    Integer  nproc = NNODES_();
    Integer  me = NODEID_();
    Integer  found = 0;
    Integer  cur_node;
    int   i, proclo, prochi;

    if (*node == me)
        Error("PROBE_ : cannot recv message from self, msgtype=", *type);

    if (*node == -1) {                /* match anyone */

        proclo = 0;
        prochi = nproc-1;
        cur_node = next_node;

    } else
        proclo = prochi = cur_node =  *node;

    for(i = proclo; i<= prochi; i++) {

        if (cur_node != me){                /* can't receive from self */
            found = MatchShmMessage(cur_node, *type); 
            if (found) break; 
        }
        cur_node = (cur_node +1)%nproc;

    }

    if(found) *node = cur_node;

    /* if wildcard node, determine which node we'll start with next time */
    if(*type == -1) next_node = (cur_node +1)%nproc;
    return(found);
}


/**
 * Return 1/0 (TRUE/FALSE) if a message of the given type is available
 * from the given node.  If the node is specified as -1, then all nodes
 * will be examined.  Some attempt is made at ensuring fairness.
 */
Integer PROBE_(Integer *type, Integer *node)
{
    Integer nnode = *node;
    Integer result;

#ifdef USE_VAMPIR
    vampir_begin(TCGMSG_PROBE,__FILE__,__LINE__);
#endif

    result = ProbeNode(type, &nnode);

#ifdef USE_VAMPIR
    vampir_end(TCGMSG_PROBE,__FILE__,__LINE__);
#endif

    return(result);
}


/**
 * Integer *type        = user defined type of received message (input)
 * char *buf         = data buffer (output)
 * Integer *lenbuf      = length of buffer in bytes (input)
 * Integer *lenmes      = length of received message in bytes (output)
 *                     (exceeding receive buffer is hard error)
 * Integer *nodeselect  = node to receive from (input)
 *                     -1 implies that any pending message of the specified
 *                     type may be received
 * Integer *nodefrom    = node message is received from (output)
 * Integer *sync        = flag for sync(1) or async(0) receipt (input)
 */
void RCV_(Integer *type, void *buf, Integer *lenbuf, Integer *lenmes, Integer *nodeselect, Integer *nodefrom, Integer *sync)
{
    static Integer ttype;
    static Integer node;
    Integer   me = NODEID_();
    void msg_rcv();

#ifdef USE_VAMPIR
    vampir_begin(TCGMSG_RCV,__FILE__,__LINE__);
#endif

    node = *nodeselect;

    ttype = *type;

    if (DEBUG_) {
        printf("RCV_: node %ld receiving from %ld, len=%ld, type=%ld, sync=%ld\n",
                (long)me, (long)*nodeselect, (long)*lenbuf, (long)*type, (long)*sync);
        fflush(stdout);
    }

    /* wait for a matching message */
    if(node==-1)   while(ProbeNode(type, &node) == 0);
    msg_rcv(ttype, buf, *lenbuf, lenmes, node); 
    *nodefrom = node;  

    if (DEBUG_) {
        (void) printf("RCV: me=%ld, from=%ld, len=%ld\n",
                      (long)me, (long)*nodeselect, (long)*lenbuf);
        (void) fflush(stdout);
    }
#ifdef USE_VAMPIR
    vampir_recv(me,*nodefrom,*lenmes,*type);
    vampir_end(TCGMSG_RCV,__FILE__,__LINE__);
#endif
}


/**
 * Integer *type     = user defined integer message type (input)
 * char *buf      = data buffer (input)
 * Integer *lenbuf   = length of buffer in bytes (input)
 * Integer *node     = node to send to (input)
 * Integer *sync     = flag for sync(1) or async(0) communication (input)
 */
void SND_(Integer *type, void *buf, Integer *lenbuf, Integer *node, Integer *sync)
{
    Integer me = NODEID_();
    Integer msg_async_snd();

    /*asynchronous communication not supported under LAPI */
#ifdef LAPI
    Integer block = 1;
#else
    Integer block = *sync;
#endif

#ifdef USE_VAMPIR
    vampir_begin(TCGMSG_SND,__FILE__,__LINE__);
    vampir_send(me,*node,*lenbuf,*type);
#endif

    if (DEBUG_) {
        (void)printf("SND_: node %ld sending to %ld, len=%ld, type=%ld, sync=%ld\n",
                     (long)me, (long)*node, (long)*lenbuf, (long)*type, (long)*sync);
        (void) fflush(stdout);
    }

    if (block)
        msg_wait(msg_async_snd(*type, buf, *lenbuf, *node));

    else {

        if (n_in_msg_q >= MAX_Q_LEN)
            Error("SND: overflowing async Q limit", n_in_msg_q);

        msg_q[n_in_msg_q].msg_id = msg_async_snd(*type, buf, *lenbuf, *node);
        msg_q[n_in_msg_q].node   = *node;
        msg_q[n_in_msg_q].type   = *type;
        n_in_msg_q++;
    }

    if (DEBUG_) {
        (void) printf("SND: me=%ld, to=%ld, len=%ld \n",
                      (long)me, (long)*node, (long)*lenbuf);
        (void) fflush(stdout);
    }

#ifdef USE_VAMPIR
    vampir_end(TCGMSG_SND,__FILE__,__LINE__);
#endif
}


int compare_msg_q_entries(const void* entry1, const void* entry2)
{
    /* nodes are nondistiguishable unless one of them is INVALID_NODE */
    if( ((struct msg_q_struct*)entry1)->node ==
            ((struct msg_q_struct*)entry2)->node)                 return 0;
    if( ((struct msg_q_struct*)entry1)->node == INVALID_NODE) return 1;
    if( ((struct msg_q_struct*)entry2)->node == INVALID_NODE) return -1;
    return 0;
}


/**
 * Wait for all messages (send/receive) to complete between
 * this node and node *nodesel or everyone if *nodesel == -1.
 */
void WAITCOM_(Integer *nodesel)
{
    Integer i, found = 0;

#ifdef USE_VAMPIR
    vampir_begin(TCGMSG_WAITCOM,__FILE__,__LINE__);
#endif

    for (i=0; i<n_in_msg_q; i++) if(*nodesel==msg_q[i].node || *nodesel ==-1){

        if (DEBUG_) {
            (void) printf("WAITCOM: %ld waiting for msgid %ld, #%ld\n",
                          (long)NODEID_(), (long)msg_q[i].msg_id, (long)i);
            (void) fflush(stdout);
        }

        msg_wait(msg_q[i].msg_id);

        msg_q[i].node = INVALID_NODE;
        found = 1;

    }else if(msg_q[i].node == INVALID_NODE)Error("WAITCOM: invalid node entry",i);

    /* tidy up msg_q if there were any messages completed  */
    if(found){

        /* sort msg queue only to move the completed msg entries to the end*/
        /* comparison tests against the INVALID_NODE key */
        qsort(msg_q, n_in_msg_q, sizeof(struct msg_q_struct),compare_msg_q_entries);

        /* update msg queue length, = the number of outstanding msg entries left*/
        for(i = 0; i< n_in_msg_q; i++)if(msg_q[i].node == INVALID_NODE) break;
        if(i == n_in_msg_q) Error("WAITCOM: inconsitency in msg_q update", i);
        n_in_msg_q = i;

    }

#ifdef USE_VAMPIR
    vampir_end(TCGMSG_WAITCOM,__FILE__,__LINE__);
#endif
}
