#include "SoftXMT.hpp"
#include "Allocator.hpp"
#include "Addressing.hpp"
#include "Cache.hpp"
#include "common.hpp"

#define BILLION 1000000000
#define MILLION 1000000

/// command line options for cache experiment
DEFINE_int64(nelems, 1<<8, "total number of elements (size == 8 bytes) to local");
//DEFINE_int64(nchunks, 1, "number of chunks to break into");
DEFINE_int64(cache_elems, 1<<5, "number of data elements in each cache block (sizeof(data_t) == 8)");

DEFINE_bool(incoherent_ro, false, "run experiment with Incoherent::RO cache (no write-back phase)");
DEFINE_bool(incoherent_rw, false, "run experiment with Incoherent::RW cache (load, increment, write-back");

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

static uint64_t total_acquire_time = 0;
static uint64_t total_release_time = 0;

static thread * main_thread = NULL;

struct chunk_result_args {
  data_t result;
  int64_t acquire_time;
  int64_t release_time;
};
typedef void (am_chunk_result_t)(chunk_result_args*, size_t, void*, size_t);
static void am_chunk_result(chunk_result_args* a, size_t asz, void* p, size_t psz) {
  total_result += a->result;
  total_acquire_time += a->acquire_time;
  total_release_time += a->release_time;
  ++replies;
  SoftXMT_wake(main_thread); // let main_thread check if we're done
}

struct process_chunk_args {
  GlobalAddress<data_t> addr;
  size_t num_elems;
  Node caller_node;
};
static void process_chunk_ro(thread * me, process_chunk_args* a) {
  Incoherent<data_t>::RO chunk(a->addr, a->num_elems);

  uint64_t start, end;
  rdtscll(start);
  chunk.block_until_acquired();
  rdtscll(end);
  int64_t acquire_time = end-start;
  DVLOG(5) << "acquire_time_ns: " << end-start << std::endl;
  
  data_t total = 0.0;
  for (int i=0; i<a->num_elems; i++) {
    total += chunk[i];
  }

  DVLOG(5) << "chunk total = " << total;
  
  chunk_result_args ra = { total, acquire_time, 0 };
  SoftXMT_call_on(a->caller_node, &am_chunk_result, &ra);
  SoftXMT_flush(a->caller_node);
  delete a;
}

static void am_spawn_process_chunk_ro(process_chunk_args* a, size_t asz, void* p, size_t psz) {
  // we can't call blocking functions from inside an active message, so spawn a thread
  process_chunk_args * aa = new process_chunk_args;
  *aa = *a;
  SoftXMT_template_spawn( &process_chunk_ro, aa );
}

static void process_chunk_rw(thread * me, process_chunk_args* a) {
  Incoherent<data_t>::RW chunk(a->addr, a->num_elems);
  
  uint64_t start, end;
  rdtscll(start);
  chunk.block_until_acquired();
  rdtscll(end);
  int64_t acquire_time = end-start;
  DVLOG(5) << "acquire_time_ns: " << end-start << std::endl;
  
  data_t total = 0.0;
  for (int i=0; i<a->num_elems; i++) {
    chunk[i]++;
  }
  
  rdtscll(start);
  chunk.block_until_released();
  rdtscll(end);
  int64_t release_time = end-start;
  
  chunk_result_args ra = { total, acquire_time, release_time };
  SoftXMT_call_on(a->caller_node, &am_chunk_result, &ra);
  SoftXMT_flush(a->caller_node);
  delete a;
}

static void am_spawn_process_chunk_rw(process_chunk_args* a, size_t asz, void* p, size_t psz) {
  // we can't call blocking functions from inside an active message, so spawn a thread
  process_chunk_args * aa = new process_chunk_args;
  *aa = *a;
  SoftXMT_template_spawn( &process_chunk_ro, aa );
}

static void cache_experiment(experiment_t exp, int64_t cache_elems) {
  main_thread = get_current_thread();
  replies = 0;
  total_result = 0;
//  total_acquire_time = 0; // remote... doesn't work
  
  size_t num_chunks = N / cache_elems;
  
  process_chunk_args * alist = new process_chunk_args[num_chunks];
  
  uint64_t start, end;
  rdtscll(start);
  
  for (int i=0; i<num_chunks; i++) {
    alist[i].addr = GlobalAddress<data_t>(data + i*cache_elems);
    alist[i].num_elems = cache_elems;
    alist[i].caller_node = 0;
    
    if (exp == INCOHERENT_RO) {
      SoftXMT_call_on(1, &am_spawn_process_chunk_ro, &alist[i]);
    } else if (exp == INCOHERENT_RW) {
      SoftXMT_call_on(1, &am_spawn_process_chunk_rw, &alist[i]);      
    }
  }
  
  while (replies < num_chunks) {
    DVLOG(5) << "waiting for replies (" << replies << "/" << num_chunks << " so far)";
    SoftXMT_suspend();
  }
  rdtscll(end);
  DVLOG(5) << "all replies received";
  DVLOG(5) << "total_result = " << total_result;
  
  std::string exp_name = (exp == INCOHERENT_RO) ? "incoherent_ro" : "incoherent_rw";
  
  LOG(INFO) << exp_name
            << ", num_chunks: " << num_chunks << ", total_read_time_ns: " << end-start
            << ", avg_acquire_time_ns: " << (double)total_acquire_time / num_chunks
            << ", avg_release_time_ns: " << (double)total_release_time / num_chunks;
  
}

static void user_main(thread * me, void * args) {
  
  if (FLAGS_incoherent_ro) {
    cache_experiment(INCOHERENT_RO, FLAGS_cache_elems);
  }
  
  if (FLAGS_incoherent_rw) {
    cache_experiment(INCOHERENT_RW, FLAGS_cache_elems);
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
