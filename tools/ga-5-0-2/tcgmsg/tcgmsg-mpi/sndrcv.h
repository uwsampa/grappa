/** @file
 * This header file declares stubs and show prototypes of the 
 * public sndrcv calls
 *
 * srftoc.h contains macros which define the names of c routines
 * accessible from FORTRAN and vice versa
 */
#ifndef SNDRCV_H_
#define SNDRCV_H_

#include "typesf2c.h"
#include "msgtypesc.h"
#include "srftoc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Integer NODEID_() returns logical node no. of current process.
 * This is 0,1,...,NNODES_()-1
 */
extern Integer NODEID_();

/**
 * Integer NNODES_() returns total no. of processes
 */
extern Integer NNODES_();

/**
 * MTIME_() return wall clock time in centiseconds
 */
extern Integer MTIME_();

/**
 * NICEFTN_() change process priority
 */
extern Integer NICEFTN_(Integer *ival);

/**
 * TCGTIME_() returns wall clock time in seconds as accurately as possible
 */
extern DoublePrecision FATR TCGTIME_();

/**
 * send message of type *type, from address buf, length *lenbuf bytes
 * to node *node.
 * Specify *sync as 1 for (mostly) synchronous, 0 for asynchronous.
 */
extern void SND_(Integer *type, void *buf, Integer *lenbuf, Integer *node, Integer *sync);

/**
 * receive a message of type *type in buffer buf, size of buf is *lenbuf
 * bytes. The actual length returned in lenmes, the sending node in 
 * nodefrom. If *nodeselect is a positve integer a message is sought
 * from that node. If it is -1 the next pending message is read.
 * Specify sync as 1 for synchronous, 0 for asynchronous.
 */
extern void RCV_(Integer *type, void *buf, Integer *lenbuf, Integer *lenmes, Integer *nodeselect, Integer *nodefrom, Integer *sync);

/**
 * Return TRUE/FALSE (1/0) if a message of the specified type is
 * available from the specified node (-1 == any node)
 */
extern Integer PROBE_(Integer *type, Integer *node);

/**
 * Broadcast to all other nodes the contents of buf, length *lenbuf
 * bytes, type *type, with the info originating from node *originator.
 */
extern void BRDCST_(Integer *type, char *buf, Integer *lenbuf, Integer *originator);

/**
 * Apply commutative global operation to elements of x on an element
 * by element basis.
 */
extern void DGOP_(Integer *type, DoublePrecision *x, Integer *n, char *op, Integer oplen);

/**
 * Apply commutative global operation to elements of x on an element
 * by element basis.
 */
extern void IGOP_(Integer *type, Integer *x, Integer *n, char *op, Integer oplen);

/**
 * void PBEGINF_()
 * This interfaces FORTRAN to the C routine pbegin. This is the first
 * thing that should be called on entering the FORTRAN main program.
 * The last thing done is to call PEND_()
 */
extern void PBEGINF_();
extern void PBGINF_();

/**
 * Initialize the parallel environment ... argc and argv contain the
 * arguments in the usual C fashion. pbegin is only called from C.
 * FORTRAN should call pbeginf which has no arguments.
 */
extern void tcgi_pbegin(int argc, char **argv);
extern void tcgi_alt_pbegin(int *argc, char ***argv);

/**
 * call to tidy up and signal master that have finished
 */
extern void FATR PEND_();

/**
 * set internal debug flag on this process to value (TRUE or FALSE)
 */
extern void SETDBG_(Integer *value);

/**
 * This call communicates with a dedicated server process and returns the 
 * next counter (value 0, 1, ...) associated with a single active loop.
 * mproc is the number of processes actively requesting values. 
 * This is used for load balancing.
 * It is used as follows:
 *
 * mproc = nproc;
 * while ( (i=nxtval(&mproc)) < top ) {
 *   do work for iteration i;
 * }
 * mproc = -mproc;
 * (void) nxtval(&mproc);
 *
 * Clearly the value from nxtval() can be used to indicate that some
 * locally determined no. of iterations should be done as the overhead
 * of nxtval() may be large (approx 0.05-0.5s per call ... so each process
 * should do about 5-50s of work per call for a 1% overhead).
 */
extern Integer NXTVAL_(Integer *mproc);

/**
 * void LLOG_() reopens stdin and stderr as log.<process no.>
 */
extern void FATR LLOG_();

/**
 * Synchronize processes with messages of given type.
 */
extern void SYNCH_(Integer *type);

/**
 * void STATS_() print out communication statitics for the current process
 */
extern void FATR STATS_();

/**
 * Wait for completion of all asynchronous send/recieve to node *node
 */
extern void WAITCOM_(Integer *node);

/**
 * Prints error message and terminates after cleaning up as much as possible. 
 * Called only from C. FORTRAN should call the routine PARERR.
 */
extern void Error(char *string, Integer integer);
#define ERROR_ Error

/**
 * FORTRAN interface to Error which is called as
 * Error("User detected error in FORTRAN", *code);
*/
void PARERR_(Integer *code);

/**
 * returns DoublePrecision precision random no. in [0.0,1.0]
 */
extern DoublePrecision FATR DRAND48_();

/**
 * sets seed of DRAND48 ... seed a positive integer
 */
extern void FATR SRAND48_(Integer *seed);

/**
 * returns no. of bytes that *n DoublePrecision occupy
 */
extern Integer MDTOB_(Integer *n);

/**
 * returns no. of bytes that *n Integers occupy
 */
extern Integer MITOB_(Integer *n);

/**
 * returns minimum no. of Integers that can hold n DoublePrecisions
 */
extern Integer MDTOI_(Integer *n);

/**
 * returns minimum no. of DoublePrecisions that can hold b Integers
 */
extern Integer MITOD_(Integer *n);

/**
 * void PFILECOPY_(Integer *type, Integer *node0, char *filename)
 * ... C interface
 * All processes call this simultaneously ... the file (unopened) on
 * process node0 is copied to all the other nodes.  Since processes
 * may be in the same directory it is recommended that distinct
 * filenames are used.
 */
extern void PFILECOPY_(Integer *type, Integer *node0, char *filename);

/**
 * void PFCOPY_(Integer *type, Integer *node0, FORT CHAR *filename)
 * ... FORTRAN interface
 *
 * All processes call this simultaneously ... the file (unopened) on
 * process node0 is copied to all the other nodes.  Since processes
 * may be in the same directory it is recommended that distinct
 * filenames are used.
 */
extern void PFCOPY_(Integer *type, Integer *node0, char *filename, int flen);

/**
 * TCGREADY tells if TCGMSG was already initialized (1) or not (0) 
 */
extern Integer TCGREADY_();

#ifdef __cplusplus
}
#endif

/*
  Miscellaneous routines for internal use only?
*/

extern void MtimeReset();
extern void PrintProcInfo();
extern void RemoteConnect();
extern void USleep();

#endif /* SNDRCV_H_ */
