#include "SoftXMT.hpp"
#include "Allocator.hpp"
#include "Addressing.hpp"
#include "Cache.hpp"

static const int64_t N = 128;

double * data;

struct process_chunk_args {
  GlobalAddress<double> addr;
  size_t num_elems;
};
static void am_process_chunk(process_chunk_args* a, size_t asz, void* p, size_t psz) {
  Node mynode = SoftXMT_mynode();
  printf("<%d> processing\n", mynode);
  
  Incoherent<double>::RO chunk(a->addr, a->num_elems);
  chunk.start_acquire();
  SoftXMT_yield();
  chunk.block_until_acquired();
    
  double total = 0.0;
  for (int i=0; i<a->num_elems; i++) {
    total += chunk[i];
  }
//  total += chunk[0];
  
  printf("<%d> chunk total = %g\n", mynode, total);
}

static void user_main(thread * me, void * args) {
  printf("user main\n");
//  GlobalAddress<double> gdata(data);
  size_t num_elems = N / SoftXMT_nodes();
  
  for (Node n=0; n<SoftXMT_nodes(); n++) {
    process_chunk_args a = { GlobalAddress<double>(data+num_elems*n), num_elems };
    SoftXMT_call_on(n, &am_process_chunk, &a);
    SoftXMT_flush(n);
  }
  
  SoftXMT_signal_done();
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
  
  // do I need this in order to keep each starter thread going? I don't think I should have to, but I did for the "scratch" program.
  while (!SoftXMT_done()) {
    SoftXMT_poll();
  }
  
  printf("<%d> finishing...\n", SoftXMT_mynode());
	SoftXMT_finish( 0 );
	// should never get here
	return 0;
}
