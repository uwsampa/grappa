///////////////////////////
// tutorial/irregular_parallelism_2.cpp
///////////////////////////
#include <Grappa.hpp>
#include <Collective.hpp>
#include <ParallelLoop.hpp>
#include <iostream>

using namespace Grappa;

bool answer(int64_t x);
int64_t children(int64_t x);
void rng_init();

bool done = false;
int64_t local_searched_count = 0;

// this proxy will be available at
// the same address on all cores
GlobalCompletionEvent joiner;

void search( int64_t x ) {
  if ( !done ) {
    local_searched_count++;
    if ( answer(x) ) {
      on_all_cores([] { done = true; });
    } else {
      int64_t num_children = children(x);

      // use `joiner` to track tasks created by the forall loop
      forall_public_async<&joiner>( x+1, num_children, [](int64_t c) {
        search(c);
      });
    }
  }
}
         


int main(int argc, char *argv[]) {
  init(&argc, &argv);


  run([]{

    on_all_cores([] { rng_init(); });

    search(0);

    // block until there are no more dynamic
    // tasks enrolled with `joiner`
    joiner.wait();

    on_all_cores([] {
      std::cout << mycore() << " searched " << local_searched_count << " nodes" << std::endl;
    });
  }); 


  finalize();
}


/* randomized tree functions */
#include <random>
DEFINE_uint64(N, 1000, "A number that determines the relative depth of the answer vertex");
std::mt19937 rng;
std::uniform_int_distribution<uint64_t> ud_N_N100;
std::uniform_int_distribution<uint64_t> ud_0_5;

void rng_init() {
  rng.seed(0x5EED);
  ud_N_N100 = std::uniform_int_distribution<uint64_t>(FLAGS_N,FLAGS_N+100);
  ud_0_5 = std::uniform_int_distribution<uint64_t>(0,5);
}

bool answer(int64_t x) {
  static int64_t a = ud_N_N100(rng);
  return x == a;
}
int64_t children(int64_t x) {
  return ud_0_5(rng);
}

