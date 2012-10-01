/*
 *         ---- The Unbalanced Tree Search (UTS) Benchmark ----
 *  
 *  Copyright (c) 2010 See AUTHORS file for copyright holders
 *
 *  This file is part of the unbalanced tree search benchmark.  This
 *  project is licensed under the MIT Open Source license.  See the LICENSE
 *  file for copyright and licensing information.
 *
 *  UTS is a collaborative project between researchers at the University of
 *  Maryland, the University of North Carolina at Chapel Hill, and the Ohio
 *  State University.  See AUTHORS file for more information.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

#include <mpi.h>
#include "uts.h"
#include "uts_dm.h"

#define NUM_NOP  20000000
#define NUM_NACK 500000
#define NUM_ACK  500000

enum uts_tags { MPIWS_WORKREQUEST = 1, MPIWS_WORKRESPONSE, MPIWS_TDTOKEN, MPIWS_STATS };

// parallel execution parameters 
int chunkSize        = 20;    // number of nodes to move to/from shared area
int polling_interval = 0;

void ws_make_progress(StealStack *s);

char * impl_getName() {
  return "Polling Engine Timer";
}

void impl_abort(int err) {
  ss_abort(err);
}


// More advanced stuff not used here:
int  impl_paramsToStr(char *strBuf, int ind) { return ind; }
int  impl_parseParam(char *param, char *value) { return 1; }
void impl_helpMessage() {}

/* Fatal error */
void ss_error(char *str, int error)
{
	fprintf(stderr, "*** [Thread %i] %s\n", ss_get_thread_num(), str);
	ss_abort(error);
}


int main(int argc, char *argv[]) {
  int i, comm_size, comm_rank;
  Node root, *children;
  double nop_t, avg_nop_t;
  double nack_t, avg_nack_t;
  double ack_t, avg_ack_t;

  StealStack *ss = ss_init(&argc, &argv);

  MPI_Comm_rank(MPI_COMM_WORLD, &comm_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);

  uts_parseParams(argc, argv);
  ss_initStats(ss);

  ss_start(sizeof(Node), chunkSize);

  /***** TIME NOP POLL OPERATIONS *****/

  if (comm_rank == 0) printf("Timing %d NOP poll operations ...\n", NUM_NOP);
  MPI_Barrier(MPI_COMM_WORLD);

  nop_t = uts_wctime();

  for (i = 0; i < NUM_NOP; i++) {
    ws_make_progress(ss);
  }

  nop_t = uts_wctime() - nop_t;

  /***** TIME NACK POLL OPERATIONS *****/

  if (comm_rank == 0) printf("Timing %d NACK poll operations ...\n", NUM_NACK);
  MPI_Barrier(MPI_COMM_WORLD);

  nack_t = uts_wctime();

  // Process 0 is the "server"
  if (comm_rank == 0) {
    extern long ctrl_sent;
    while (ctrl_sent < (NUM_NACK/(comm_size-1))*(comm_size-1))
      ws_make_progress(ss);
    
    //printf(" -- process 0 done, sent %d NACK messages\n", ctrl_sent);
  }
  
  // Everyone else sends requests to process 0
  else {
    MPI_Status stat;
    long out_buff = comm_rank;
    void *in_buff = malloc(ss->chunk_size*ss->work_size);
    for (i = 0; i < NUM_NACK/(comm_size-1); i++) {
      MPI_Send(&out_buff, 1, MPI_LONG, 0, MPIWS_WORKREQUEST, MPI_COMM_WORLD);
      MPI_Recv(in_buff, ss->chunk_size*ss->work_size, MPI_BYTE, 0, MPIWS_WORKRESPONSE,
          MPI_COMM_WORLD, &stat);
    }
    //printf(" -- process %d done, recv'd %d NACK messages\n", comm_rank, i);
    free(in_buff);
  }

  nack_t = uts_wctime() - nack_t;

  /***** TIME ACK POLL OPERATIONS *****/

  if (comm_rank == 0) printf("Timing %d ACK poll operations ...\n", NUM_ACK);
  MPI_Barrier(MPI_COMM_WORLD);

  if (comm_rank == 0)  {
    // Put NUM_ACK chunks of work into our queue
    Node node;
    int i;
    
    uts_initRoot(&node, 0);
    node.numChildren = 0;
   
    for (i = 0; i < NUM_ACK*ss->chunk_size; i++)
      ss_put_work(ss, &node);
  }

  MPI_Barrier(MPI_COMM_WORLD);


  ack_t = uts_wctime();

  // Process 0 is the "server"
  if (comm_rank == 0) {
    extern long chunks_sent;
    extern long ctrl_sent;
    long old_sent = ctrl_sent;

    while (ctrl_sent-old_sent < comm_size-1) {
      ws_make_progress(ss);
    }
    
    //printf(" -- process 0 done, sent %d chunks of work\n", chunks_sent);
  }
  
  // Everyone else sends requests to process 0
  else {
    MPI_Status stat;
    int  work_rcv, nchunks = 0;
    long out_buff = comm_rank;
    void *in_buff = malloc(ss->chunk_size*ss->work_size);

    do {
      MPI_Send(&out_buff, 1, MPI_LONG, 0, MPIWS_WORKREQUEST, MPI_COMM_WORLD);
      MPI_Recv(in_buff, ss->chunk_size*ss->work_size, MPI_BYTE, 0, MPIWS_WORKRESPONSE,
          MPI_COMM_WORLD, &stat);

      MPI_Get_count(&stat, MPI_BYTE, &work_rcv);
      ++nchunks;
    } while (work_rcv > 0);

    //printf(" -- process %d done, recv'd %d chunks of work\n", comm_rank, nchunks);
    free(in_buff);
  }

  ack_t = uts_wctime() - ack_t;

  /***** GATHER UP RESULTS *****/

  ss_stop();

  MPI_Reduce(&nop_t , &avg_nop_t, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

  if (comm_rank == 0) {
    avg_nop_t  /= comm_size;
    avg_nack_t  = nack_t;
    avg_ack_t   = ack_t;

    printf("NOP  Polling time = %.5f sec, %.5e sec/op, %.2f ops/sec\n", 
      avg_nop_t, avg_nop_t/NUM_NOP, NUM_NOP/avg_nop_t);
    printf("NACK Polling time = %.5f sec, %.5e sec/op, %.2f ops/sec\n", 
      avg_nack_t, avg_nack_t/NUM_NACK, NUM_NACK/avg_nack_t);
    printf("ACK  Polling time = %.5f sec, %.5e sec/op, %.2f ops/sec\n", 
      avg_ack_t, avg_ack_t/NUM_ACK, NUM_ACK/avg_ack_t);
  }

  MPI_Finalize();

  return 0;
}
