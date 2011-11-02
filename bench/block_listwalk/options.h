
#ifndef __OPTIONS_H__
#define __OPTIONS_H__

#include <getopt.h>

struct options {
  int sockets;
  int cores;
  int threads;
  unsigned int payload_size_log;
  unsigned int payload_size;
  unsigned int count;
  int outstanding;
  int rdma_outstanding;
  int batch_size;
  int messages;
  int rdma_read;
  int rdma_write;
  int fetch_and_add;
  int contexts;
  int protection_domains;
  int queue_pairs;
  int same_destination;
  unsigned int list_size_log;
  unsigned int list_size;
  int jumpthreads;
  int id;
  int force_local;
  char * type;
};

struct options parse_options( int * argc, char ** argv[] );

#endif
