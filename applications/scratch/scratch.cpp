#include "SoftXMT.hpp"
#include "Allocator.hpp"
#include "Addressing.hpp"
#include "Delegate.hpp"
#include <iostream>
#include <vector>

#include <boost/foreach.hpp>

// set to 1 when it's time to barrier
int barrier = 0;

// set to 1 when it's time to tear down
int done = 0;

static double fun(double x) {
  return x*x;
}

double local_total = 0;
double global_total = 0;
int64_t reported_in = 0;
int64_t sub_reported_in = 0;

struct inc_total_args {
  double inc;
};
static void inc_total(inc_total_args * a, size_t asz, void * p, size_t psz) {
  global_total += a->inc;
  reported_in++;
  if (reported_in >= SoftXMT_nodes()) {
    printf("total = %g\n", global_total);
  } else {
    printf("subtotal = %g\n", global_total);
  }
}

struct scratch_args {
  int id;
  int num;
  double x1;
	double x2;
  double* result;
  scratch_args(int id, int num, double x1, double x2, double* result):
    id(id), num(num), x1(x1), x2(x2), result(result) {}
};
static void scratch_work( thread * me, void * args) {
  scratch_args& a = *((scratch_args*)args);

  double y1 = fun(a.x1);
  double y2 = fun(a.x2);
  double area = (a.x2-a.x1) * ((y2+y1)/2);
  
  *a.result += area;
  sub_reported_in++;
  
  if (sub_reported_in == a.num) {
    // better as a Delegate request to fetch_add?
    // FIXME: why doesn't this work? inc_total doesn't get called, why?
    inc_total_args ia = {local_total};
    SoftXMT_call_on(0, &inc_total, &ia);
    SoftXMT_flush(0);
    printf("<%d> local_total = %g\n", SoftXMT_mynode(), local_total);
  }
  
  delete &a;
}

struct node_work_args {
  double start;
  double end;
  int num;
};
static void node_work(node_work_args * a, size_t arg_size, void * payload, size_t payload_size) {
  int mynode = SoftXMT_mynode();
  
  double s = a->start, e = a->end;
  double w = (e-s)/a->num;
  
  for (int i=0; i<a->num; i++) {
    SoftXMT_spawn(&scratch_work, new scratch_args(i, a->num, s+i*w, s+(i+1)*w, &local_total));
  }
  printf("<%d> done spawning\n", mynode);
}

static void user_main(thread * me, void * args) {
  int mynode = SoftXMT_mynode();
  int nodes = SoftXMT_nodes();
  printf("beginning user main on <%d>...\n", mynode);
    
  for (Node n=0; n<nodes; n++) {
    node_work_args args = {(double)n/nodes, (double)(n+1)/nodes, 8};
    SoftXMT_call_on(n, &node_work, &args);
    SoftXMT_flush(n);
  }
  
  SoftXMT_signal_done();
}

int main(int argc, char * argv[]) {
	SoftXMT_init(&argc, &argv);
  SoftXMT_activate();
  
  int nodes = SoftXMT_nodes();
  size_t memsize = 1024;
  char mem[memsize];
  
  Allocator al(&mem, memsize);
  
//  global_totals = (double*)al.malloc(nodes * sizeof(double));
//  memset(global_totals, 0, nodes*sizeof(double));
  
  SoftXMT_run_user_main(&user_main, NULL);
  
  
  // TODO: what's supposed to go here in main (in general?)
  if (SoftXMT_mynode() == 0) {
    while (reported_in < nodes) {
      // need to poll in order to have Node 0 receive the inc_total am's
      SoftXMT_poll();
    }
  }
  
  printf("<%d> finishing...\n", SoftXMT_mynode());
	SoftXMT_finish( 0 );
	// should never get here
	return 0;
}
