
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "options.h"

static struct option long_options[] = {
  {"sockets",          required_argument, NULL, 'k'},
  {"cores",            required_argument, NULL, 'c'},
  {"threads", required_argument, NULL, 't'},
  {"payload_size_log", required_argument, NULL, 'l'},
  {"payload_size",     required_argument, NULL, 's'},
  {"list_size_log", required_argument, NULL, 'L'},
  {"list_size",     required_argument, NULL, 'S'},
  {"count",            required_argument, NULL, 'n'},
  {"outstanding",      required_argument, NULL, 'o'},
  {"rdma_outstanding", required_argument, NULL, 'p'},
  {"batch_size",       required_argument, NULL, 'b'},
  {"messages",         no_argument,       NULL, 'm'},
  {"rdma_read",        no_argument,       NULL, 'r'},
  {"rdma_write",       no_argument,       NULL, 'w'},
  {"fetch_and_add",    no_argument,       NULL, 'f'},

  {"jumpthreads",    no_argument,       NULL, 'J'},

  {"contexts",         required_argument, NULL, 'x'},
  {"protection_domains", required_argument, NULL, 'd'},
  {"queue_pairs",      required_argument, NULL, 'q'},
  {"same_destination",   no_argument, NULL, '0'},

  {"force_local",   no_argument, NULL, 'F'},

  {"id",         required_argument, NULL, 'I'},

  {"type",         required_argument, NULL, 'T'},
  {"pause_for_debugger",         no_argument, NULL, 'P'},

  {"help",             no_argument,       NULL, 'h'},
  {NULL,               no_argument,       NULL, 0}
};

struct options parse_options( int * argc, char ** argv[] ) {
  struct options opt = {
    .cores		= 1,
    .sockets		= 1,
    .threads	= 1,
    .payload_size_log	= 0,
    .payload_size	= 1,
    .count		= 1,
    .outstanding	= 50,
    .rdma_outstanding	= 16,
    .batch_size 	= 1,
    .messages		= 0,
    .rdma_read		= 0,
    .rdma_write		= 0,
    .fetch_and_add	= 0,
    .contexts           = 1,
    .protection_domains = 1,
    .queue_pairs        = 1,
    .same_destination   = 0,
    .list_size_log	= 0,
    .list_size	= 1,
    .jumpthreads = 1,
    .force_local = 0,
    .id = 1,
    .pause_for_debugger = 0,
  };
  strcpy( opt.type, "default" );
  int c, option_index = 1;
  while ((c = getopt_long(*argc, *argv, "k:c:t:l:s:n:o:p:b:x:d:q:L:S:I:T:PF0mrwfJh?",
                          long_options, &option_index)) >= 0) {
    switch (c) {
    case 0:   // flag set
      break;
    case 'k':
      opt.sockets = atoi(optarg);
      break;
    case 'c':
      opt.cores = atoi(optarg);
      break;
    case 't':
      opt.threads = atoi(optarg);
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
    case 'J':
      opt.jumpthreads = 1;
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
    case '0':
      opt.same_destination = 1;
      break;
    case 'L':
      opt.list_size_log = atoi(optarg);
      opt.list_size = 1L << opt.list_size_log;
      //if (opt.count == 0) opt.count = opt.list_size;
      break;
    case 'S':
      opt.list_size = atoi(optarg);
      //if (opt.count == 0) opt.count = opt.list_size;
      break;
    case 'F':
      opt.force_local = 1;
      break;
    case 'I':
      opt.id = atoi(optarg);
      break;
    case 'T':
      strncpy( opt.type, optarg, sizeof(opt.type) );
      break;
    case 'P':
      opt.pause_for_debugger = 1;
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
