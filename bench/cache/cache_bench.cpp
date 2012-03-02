#include "SoftXMT.hpp"
#include "Allocator.hpp"
#include "Addressing.hpp"
#include "Cache.hpp"

static const int64_t N = 128;

struct chunk_result_args {
  double result;
};
typedef void (am_chunk_result_t)(chunk_result_args*, size_t, void*, size_t);
static void am_chunk_result(chunk_result_args* a, size_t asz, void* p, size_t psz);

double * data;

struct process_chunk_args {
  GlobalAddress<double> addr;
  size_t num_elems;
  GlobalAddress<am_chunk_result_t> return_addr;
};
static void process_chunk( thread * me, process_chunk_args * a) {
  Node mynode = SoftXMT_mynode();
  //printf("<%d> processing\n", mynode);
  LOG(INFO) << mynode << " processing";
  
  Incoherent<double>::RO chunk(a->addr, a->num_elems);
  // chunk.start_acquire();
  // SoftXMT_yield();
  chunk.block_until_acquired();
    
  double total = 0.0;
  for (int i=0; i<a->num_elems; i++) {
    total += chunk[i];
  }
//  total += chunk[0];
  
  //printf("<%d> chunk total = %g\n", mynode, total);
  LOG(INFO) << mynode << " chunk total = " << total;
  chunk_result_args ra = { total };
  SoftXMT_call_on( a->return_addr.node(), a->return_addr.pointer(), &ra );
  SoftXMT_flush( a->return_addr.node() ); // send immediately, since there's no more work

  delete a;
}
static void am_spawn_process_chunk(process_chunk_args* a, size_t asz, void* p, size_t psz) {
  // we can't call blocking functions from inside an active message, so spawn a thread
  process_chunk_args * aa = new process_chunk_args;
  *aa = *a;
  SoftXMT_template_spawn( &process_chunk, aa );
}



static thread * main_thread = NULL;
static double result = 0.0;
static double replies = 0;
static void am_chunk_result(chunk_result_args* a, size_t asz, void* p, size_t psz) {
  result += a->result;
  ++replies;
  SoftXMT_wake( main_thread );
}


static void user_main(thread * me, void * args) {
  LOG(INFO) << "user main";
  main_thread = get_current_thread();
  
//  GlobalAddress<double> gdata(data);
  size_t num_elems = N / SoftXMT_nodes();

  for (Node n=0; n<SoftXMT_nodes(); n++) {
    process_chunk_args a = { GlobalAddress<double>(data+num_elems*n), 
                             num_elems,
                             make_global(&am_chunk_result) };
    SoftXMT_call_on(n, &am_spawn_process_chunk, &a);
    SoftXMT_flush(n); // send immediately, since there's no work yet
  }
  
  LOG(INFO) << "started all requests; waiting for replies";
  while( replies < SoftXMT_nodes() ) {
    DVLOG(5) << "waiting for replies.";
    SoftXMT_suspend();
    DVLOG(5) << "woke up; got " << replies << " of " << SoftXMT_nodes() << " replies.";
  }
  LOG(INFO) << "all replies received; finishing";

  SoftXMT_signal_done(); // kills all servers. don't call until ready to quit
}

int main(int argc, char * argv[]) {
	SoftXMT_init(&argc, &argv);
  SoftXMT_activate();
  
  size_t memsize = 1024;
  char mem[memsize];
  Allocator a(&mem, memsize);
  
  Node mynode = SoftXMT_mynode();
  if (mynode == 0) {
    data = (double*)a.malloc(N * sizeof(double));
    for (int i=0; i<N; i++) { data[i] = 1.0; }
  }
  
  SoftXMT_run_user_main(&user_main, NULL);
  
  // // do I need this in order to keep each starter thread going? I don't think I should have to, but I did for the "scratch" program.
  // while (!SoftXMT_done()) {
  //   SoftXMT_poll();
  // }
  
  //printf("<%d> finishing...\n", SoftXMT_mynode());
  LOG(INFO) << mynode << " finishing.";
  SoftXMT_finish( 0 );
  // should never get here
  return 0;
}
