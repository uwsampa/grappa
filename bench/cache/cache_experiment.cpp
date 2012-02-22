#include "SoftXMT.hpp"
#include "Allocator.hpp"
#include "Addressing.hpp"
#include "Cache.hpp"
#include "common.hpp"
#include "timer.h"

/// command line options for cache experiment
DEFINE_int64(nelems, 1<<8, "total number of elements (size == 8 bytes) to local");
//DEFINE_int64(nchunks, 1, "number of chunks to break into");
DEFINE_int64(cache_elems, 1<<5, "number of data elements in each cache block (sizeof(data_t) == 8)");
DEFINE_int64(num_threads, 8, "number of threads (per core/node for now)");

DEFINE_bool(incoherent_ro, false, "run experiment with Incoherent::RO cache (no write-back phase)");
DEFINE_bool(incoherent_rw, false, "run experiment with Incoherent::RW cache (load, increment, write-back");
DEFINE_bool(incoherent_all_remote, false, "run experiment where each thread caches the entire data set in chunks");

static const size_t memsize = 1 << 20;
static int64_t N = 0;

typedef enum {
  INCOHERENT_RO,
  INCOHERENT_RW
} experiment_t;

typedef int64_t data_t;

static data_t* data;
static data_t total_result = 0;
static int64_t replies = 0;

static double total_acquire_time = 0;

static thread * main_thread = NULL;

struct chunk_result_args {
  data_t result;
};
typedef void (am_chunk_result_t)(chunk_result_args*, size_t, void*, size_t);
static void am_chunk_result(chunk_result_args* a, size_t asz, void* p, size_t psz) {
  total_result += a->result;
  ++replies;
  SoftXMT_wake(main_thread); // let main_thread check if we're done
}

// all-to-all experiment
struct process_all_args {
  GlobalAddress<data_t> addr;
  int64_t num_elems;
  int64_t cache_elems;
  Node caller_node;
};
static void process_chunk_all(thread * me, process_all_args* a) {
  int64_t num_chunks = a->num_elems / a->cache_elems;
  data_t total = 0;
  
  data_t * buff = new data_t[a->cache_elems];
  assert(buff != NULL);

  for (int i=0; i<num_chunks; i++) {
    Incoherent<data_t>::RO chunk(a->addr, a->cache_elems, buff);
    chunk.block_until_acquired();
    
    for (int i=0; i<a->cache_elems; i++) {
      total += chunk[i];
    }
    
    chunk.block_until_released();
  }
  
  total_result += total;
  DVLOG(5) << "total: " << total;

//  chunk_result_args ra = { total };
//  SoftXMT_call_on(a->caller_node, &am_chunk_result, &ra);
//  SoftXMT_flush(a->caller_node);
}

struct spawn_all_args {
  data_t* data;
  int64_t num_threads;
  int64_t num_elems;
  int64_t cache_elems;
  Node caller;
};
static void th_spawn_all(thread * me, spawn_all_args* a) {
  // we can't call blocking functions from inside an active message, so spawn a thread
  process_all_args * alist = new process_all_args[a->num_threads];
  thread ** ths = new thread*[a->num_threads];
  
  DVLOG(5) << "spawning " << a->num_threads << " threads";
  
  total_result = 0; // zero out 'total_result' on this node
  
  double start, end;
  start = timer();
  
  for (int i=0; i<a->num_threads; i++) {
    alist[i].addr = GlobalAddress<data_t>(a->data+i*a->num_elems, a->caller);
    alist[i].num_elems = a->num_elems;
    alist[i].cache_elems = a->cache_elems;
    alist[i].caller_node = a->caller;
    
    ths[i] = SoftXMT_template_spawn( &process_chunk_all, &alist[i] );
  }
  
  for (int i=0; i<a->num_threads; i++) {
    SoftXMT_join(ths[i]);
  }
  
  end = timer();
  LOG(INFO) << "remote time: " << end-start;
  
  chunk_result_args ra = { total_result };
  SoftXMT_call_on(a->caller, &am_chunk_result, &ra);
  SoftXMT_flush(a->caller);
}

static void am_spawn_all(spawn_all_args* a, size_t asz, void* p, size_t psz) {
  spawn_all_args * aa = new spawn_all_args;
  *aa = *a;
  SoftXMT_template_spawn(&th_spawn_all, aa);
}

static void cache_experiment_all(int64_t cache_elems, int64_t num_threads) {
  main_thread = get_current_thread();
  replies = 0;
  total_result = 0;
  total_acquire_time = 0;
  
  int64_t num_elems = N / num_threads;
  
  if (num_elems/cache_elems == 0) {
    std::cerr << "too small!" << std::endl;
    exit(1);
  }
  
  double start, end;
  start = timer();
  
  spawn_all_args a = { data, num_threads, num_elems, cache_elems, 0 };
  SoftXMT_call_on(1, &am_spawn_all, &a);
  
  while (replies < 1) {
    DVLOG(5) << "waiting for replies (" << replies << "/" << num_threads << " so far)";
    SoftXMT_suspend();
  }
  end = timer();
  double all_time = end-start;
  DVLOG(5) << "all replies received";
  DVLOG(5) << "total_result = " << total_result;
  assert(total_result - (double)N < 1.0e-10);
  
  LOG(INFO) << "total_result = " << total_result;
  
  LOG(INFO)
    << "{ experiment: 'incoherent_all_remote'"
    << ", total_read_s: " << all_time
    << ", all_bw_wps: " << (N*2)/(double)(all_time)
    << " }";
}

static void user_main(thread * me, void * args) {
  
  if (FLAGS_incoherent_all_remote) {
    cache_experiment_all(FLAGS_cache_elems, FLAGS_num_threads);
  }
  
  LOG(INFO) << "done with experiments...";
  SoftXMT_signal_done();
}

int main(int argc, char * argv[]) {
  SoftXMT_init(&argc, &argv);
  SoftXMT_activate();
  N = FLAGS_nelems;
//  size_t memsize = (N*sizeof(data_t)*2); // x2 just to be on the safe side
//  char * mem = new char[memsize];
//  Allocator a(&mem[0], memsize);
  
  Node mynode = SoftXMT_mynode();
  if (mynode == 0) {
//    data = (data_t*)a.malloc(N * sizeof(data_t));
    data = (data_t*)malloc(N*sizeof(data_t));
    
    for (int i=0; i<N; i++) { data[i] = 1; }
  }
  
  SoftXMT_run_user_main(&user_main, NULL);

  LOG(INFO) << "finishing";
	SoftXMT_finish( 0 );
	// should never get here
	return 0;
}
