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

static double total_work_time = 0;

static thread * main_thread = NULL;

struct chunk_result_args {
  data_t result;
  double work_time;
};
typedef void (am_chunk_result_t)(chunk_result_args*, size_t, void*, size_t);
static void am_chunk_result(chunk_result_args* a, size_t asz, void* p, size_t psz) {
  total_result += a->result;
  total_work_time += a->work_time;
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
  
  data_t buff[1024];
  assert(buff != NULL);

  for (int i=0; i<num_chunks; i++) {
    Incoherent<data_t>::RW chunk(a->addr, a->cache_elems, buff);
    chunk.block_until_acquired();
    
    for (int i=0; i<a->cache_elems; i++) {
      total += chunk[i];
    }
    
    chunk.block_until_released();
  }
  
  total_result += total;
  DVLOG(2) << "<" << SoftXMT_mynode() << "> " << "total: " << total;

//  chunk_result_args ra = { total };
//  SoftXMT_call_on(a->caller_node, &am_chunk_result, &ra);
//  SoftXMT_flush(a->caller_node);
}

struct spawn_all_args {
  GlobalAddress<data_t> data;
  int64_t num_threads;
  int64_t num_elems;
  int64_t cache_elems;
  Node caller;
};
static void th_spawn_all(thread * me, spawn_all_args* a) {
  // we can't call blocking functions from inside an active message, so spawn a thread
  process_all_args * alist = new process_all_args[a->num_threads];
  thread ** ths = new thread*[a->num_threads];
  
  DVLOG(1) << "spawning " << a->num_threads << " threads";
  
  total_result = 0; // zero out 'total_result' on this node
  
  double start, end;
  start = timer();
  
  for (int i=0; i<a->num_threads; i++) {
    alist[i].addr = a->data;
    alist[i].num_elems = a->num_elems;
    alist[i].cache_elems = a->cache_elems;
    alist[i].caller_node = a->caller;
    
    ths[i] = SoftXMT_template_spawn( &process_chunk_all, &alist[i] );
  }
  
  for (int i=0; i<a->num_threads; i++) {
    SoftXMT_join(ths[i]);
  }
  
  end = timer();
  double work_time = end-start;
  DVLOG(1) << "work time: " << work_time;
  
  chunk_result_args ra = { total_result, work_time };
  SoftXMT_call_on(a->caller, &am_chunk_result, &ra);
  SoftXMT_flush(a->caller);
  delete a;
}

struct exp_args {
  Node target;
  int64_t cache_size;
  int num_threads;
  int64_t elems_per_thread;
};
/// thread that initiates the data transfer between two nodes
static void th_experiment_node(thread * me, exp_args * a) {
  DVLOG(1) << "th_experiment_node";
  replies = 0;
  total_result = 0;
  total_work_time = 0;
  
  data = new data_t[a->cache_size];
  for (int i=0; i<a->cache_size; i++) { data[i] = 1; }
  
  DVLOG(1) << "starting transfer from " << SoftXMT_mynode() << " to " << a->target;

  if (a->elems_per_thread/a->cache_size == 0) {
    std::cerr << "too small!" << std::endl;
    exit(1);
  }
  
  Node caller = 0; // send all responses back to one node
  spawn_all_args aa = { make_global(data), a->num_threads, a->elems_per_thread, a->cache_size, caller};
  SoftXMT_remote_spawn(&th_spawn_all, &aa, a->target);
  
}
static void cache_experiment_all() {
  int64_t nt = FLAGS_num_threads;
  int64_t cache_size = FLAGS_cache_elems;
  int64_t N = FLAGS_nelems;
  Node nodes = SoftXMT_nodes();
  Node nstarters = nodes/2;
  int64_t elems_per_thread = N / nt;

  DVLOG(1) << "starting experiments across all nodes";
  replies = 0;

  double start, end;
  start = timer();
  
  exp_args a[nstarters];
  for (int i = 0; i < nstarters; i++) {
    a[i].target = nstarters+i;
    a[i].cache_size = cache_size;
    a[i].num_threads = (int)nt;
    a[i].elems_per_thread = elems_per_thread;

    SoftXMT_remote_spawn(&th_experiment_node, &a[i], i);
    SoftXMT_flush(i); // should be the only message going to that node
  }
  
  if (SoftXMT_mynode() == 0) {
    while (replies < nstarters) {
      DVLOG(1) << "waiting for replies (" << replies << "/" << nstarters << " so far)";
      SoftXMT_suspend();
    }
    total_result /= nstarters; // 'average' result

    end = timer();
    double all_time = end-start;
    DVLOG(1) << "all replies received";
    DVLOG(1) << "total_result = " << total_result;
    assert(total_result - (double)N < 1.0e-10);
    
    LOG(INFO) << "total_result = " << total_result;
    
    LOG(INFO) << "{ experiment: 'incoherent_all_remote'"
              << ", total_work_s: " << total_work_time
              << ", work_bw_wps: " << (2*elems_per_thread*nt) / total_work_time
              << ", total_read_s: " << all_time
              << ", all_bw_wps: " << (2*elems_per_thread*nt)/(double)(all_time)
              << " }";
  }
  DVLOG(1) << SoftXMT_mynode() << " done with exp";
}

static void user_main(thread * me, void * args) {
  main_thread = get_current_thread();

  if (FLAGS_incoherent_all_remote) {
    cache_experiment_all();
  }
  
  SoftXMT_signal_done();
}

int main(int argc, char * argv[]) {
  SoftXMT_init(&argc, &argv);
  SoftXMT_activate();
//  N = FLAGS_nelems;
//  size_t memsize = (N*sizeof(data_t)*2); // x2 just to be on the safe side
//  char * mem = new char[memsize];
//  Allocator a(&mem[0], memsize);
  
  SoftXMT_run_user_main(&user_main, NULL);

  LOG(INFO) << "finishing";
	SoftXMT_finish( 0 );
	// should never get here
	return 0;
}
