
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "options.h"

static struct option long_options[] = {
  {"cores",            required_argument, NULL, 'c'},
  {"threads_per_core", required_argument, NULL, 't'},
  {"payload_size_log", required_argument, NULL, 'l'},
  {"payload_size",     required_argument, NULL, 's'},
  {"count",            required_argument, NULL, 'n'},
  {"outstanding",      required_argument, NULL, 'o'},
  {"rdma_outstanding", required_argument, NULL, 'p'},
  {"batch_size",       required_argument, NULL, 'b'},
  {"messages",         no_argument,       NULL, 'm'},
  {"rdma_read",        no_argument,       NULL, 'r'},
  {"rdma_write",       no_argument,       NULL, 'w'},
  {"fetch_and_add",    no_argument,       NULL, 'f'},

  {"contexts",         required_argument, NULL, 'x'},
  {"protection_domains", required_argument, NULL, 'd'},
  {"queue_pairs",      required_argument, NULL, 'q'},

  {"help",             no_argument,       NULL, 'h'},
  {NULL,               no_argument,       NULL, 0}
};

struct options parse_options( int * argc, char ** argv[] ) {
  struct options opt = {
    .cores		= 1,
    .threads_per_core	= 1,
    .payload_size_log	= 0,
    .payload_size	= 1,
    .count		= 1 << 10,
    .outstanding	= 50,
    .rdma_outstanding	= 16,
    .batch_size 	= 5,
    .messages		= 0,
    .rdma_read		= 0,
    .rdma_write		= 0,
    .fetch_and_add	= 0,
    .contexts           = 1,
    .protection_domains = 1,
    .queue_pairs        = 1,
  };
  int c, option_index = 1;
  while ((c = getopt_long(*argc, *argv, "c:t:l:s:n:o:p:b:x:d:q:mrwfh?",
                          long_options, &option_index)) >= 0) {
    switch (c) {
    case 0:   // flag set
      break;
    case 'c':
      opt.cores = atoi(optarg);
      break;
    case 't':
      opt.threads_per_core = atoi(optarg);
      break;
    case 'l':
      opt.payload_size_log = atoi(optarg);
      opt.payload_size = 1L << opt.payload_size_log;
      break;
    case 's':
      opt.payload_size = atoi(optarg);
      break;
    case 'n':
      opt.count = atoi(optarg);
      break;
    case 'o':
      opt.outstanding = atoi(optarg);
      break;
    case 'p':
      opt.rdma_outstanding = atoi(optarg);
      break;
    case 'b':
      opt.batch_size = atoi(optarg);
      break;
    case 'm':
      opt.messages = 1;
      break;
    case 'r':
      opt.rdma_read = 1;
      break;
    case 'w':
      opt.rdma_write = 1;
      break;
    case 'f':
      opt.fetch_and_add = 1;
      break;
    case 'x':
      opt.contexts = atoi(optarg);
      break;
    case 'd':
      opt.protection_domains = atoi(optarg);
      break;
    case 'q':
      opt.queue_pairs = atoi(optarg);
      break;
    case 'h':
    case '?':
    default: 
      {
	printf("Available options:\n");
	struct option* optp;
	for (optp = &long_options[0]; optp->name != NULL; ++optp) {
	  if (optp->has_arg == no_argument) {
	    printf("  -%c, --%s\n", optp->val, optp->name);
	  } else {
	    printf("  -%c, --%s <ARG>\n", optp->val, optp->name);
	  }
	}
	exit(1);
      }
    }
  }
  return opt;
}
