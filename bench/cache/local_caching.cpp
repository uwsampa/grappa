#include "SoftXMT.hpp"
#include "Allocator.hpp"
#include "Addressing.hpp"
#include "Cache.hpp"
#include "common.hpp"

#define BILLION 1000000000
#define MILLION 1000000

inline std::ostream& node_cout() {
  return std::cout << "<" << SoftXMT_mynode() << ">";
}

static const size_t memsize = 1 << 20;
static const int64_t N = 1<<8;

typedef int64_t data_t;

data_t* data;

struct process_chunk_args {
  GlobalAddress<data_t> addr;
  size_t num_elems;
};
static void am_process_chunk(process_chunk_args* a, size_t asz, void* p, size_t psz) {
  Node mynode = SoftXMT_mynode();
  printf("<%d> processing\n", mynode);
  
  Incoherent<data_t>::RO chunk(a->addr, a->num_elems);
  chunk.start_acquire();
  SoftXMT_yield();
  chunk.block_until_acquired();
  
  double total = 0.0;
  for (int i=0; i<a->num_elems; i++) {
    total += chunk[i];
  }
  
  printf("<%d> chunk total = %g\n", mynode, total);
}

static void th_process_chunk(thread * me, void * args) {
  process_chunk_args * a = (process_chunk_args*)args;
  
//  Node mynode = SoftXMT_mynode();
//  printf("<%d> processing\n", mynode);
  
  Incoherent<data_t>::RO chunk(a->addr, a->num_elems);
  
//  uint64_t start, end;
//  rdtscll(start);
  chunk.start_acquire();
  SoftXMT_yield();
  chunk.block_until_acquired();
//  rdtscll(end);
  
//  std::cout << "acquire_time_ns: " << end-start << std::endl;
  
//  data_t total = 0;
//  for (int i=0; i<a->num_elems; i++) {
//    total += chunk[i];
//  }
  
//  node_cout() << " chunk_total = " << total << std::endl;  
}

static uint64_t exp_cache(size_t num_chunks) {
  size_t num_elems = N / num_chunks;
  
  process_chunk_args * alist = (process_chunk_args*)malloc(num_chunks*sizeof(process_chunk_args));
  
  uint64_t start, end;
  rdtscll(start);
  
  for (int i=0; i<num_chunks; i++) {
    alist[i].addr = GlobalAddress<data_t>(data + i*num_elems);
    alist[i].num_elems = num_elems;
    //    SoftXMT_call_on(0, &am_process_chunk, &a);
    //    SoftXMT_flush(0);
    //    SoftXMT_spawn(&th_process_chunk, &alist[i]);
    th_process_chunk(current_thread, &alist[i]);
  }
  
  rdtscll(end);
  
  return end-start;
}

static void user_main(thread * me, void * args) {  
#define do_exp(n) \
  { \
    uint64_t time = exp_cache(n); \
    std::cout << "num_chunks: " << (n) << ", " << "total_read_time_ns: " << time << std::endl; \
  }
  
  do_exp(1);
  do_exp(1<<4);
  do_exp(1<<6);
  do_exp(N);
  
#undef do_exp
  SoftXMT_signal_done();
}

int main(int argc, char * argv[]) {
	SoftXMT_init(&argc, &argv);
  SoftXMT_activate();
  
  char mem[memsize];
  Allocator a(&mem, memsize);
  
  Node mynode = SoftXMT_mynode();
  if (mynode == 0) {
    data = (data_t*)a.malloc(N * sizeof(data_t));
    for (int i=0; i<N; i++) { data[i] = 1; }
  }
  
  SoftXMT_run_user_main(&user_main, NULL);
  
  // do I need this in order to keep each starter thread going? I don't think I should have to, but I did for the "scratch" program.
  while (!SoftXMT_done()) {
    SoftXMT_poll();
  }
  
  printf("<%d> finishing...\n", SoftXMT_mynode());
	SoftXMT_finish( 0 );
	// should never get here
	return 0;
}
