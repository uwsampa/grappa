#include <iostream>

#include <cassert>
#include "SoftXMT.hpp"
#include "Delegate.hpp"
#include "common.hpp"

#define BILLION 1000000000
#define MILLION 1000000

const uint64_t data_size = 1<<10;
int64_t some_data[data_size];

struct user_main_args {
    int argc;
    char** argv;
};

struct sender_args {
    int64_t* base;
    uint64_t num;
};

void sending_task( thread* me, void* args ) {
    sender_args* sargs = (sender_args*) args;
    int64_t* base = sargs->base;
    uint64_t num = sargs->num;

    std::cout << current_thread->id << " starts sending" << std::endl;
    for (uint64_t i=0; i<num; i++) {
        std::cout << current_thread->id << " :: " << i << std::endl;
        *(base + i) = SoftXMT_delegate_read_word( base + i );
    }
    std::cout << current_thread->id << " finished sending" << std::endl;
}

void user_main( thread * me, void * args ) {
 
    user_main_args* umargs = (user_main_args*) args;
    std::cout << umargs->argv << std::endl;
    uint64_t num_threads = atoi(umargs->argv[1]); 
    
    int rank = SoftXMT_mynode();
    assert ( SoftXMT_nodes() == 2 );

//    // initialize my data contents 
//    std::cout << rank << " initializing contents" << std::endl;
//    for (uint64_t i=0; i<data_size; i++) {
//        some_data[data_size] = rand();
//    }

    uint64_t size_per_thread = data_size / num_threads;

    // spawn sender threads
    thread* sender_threads[num_threads];
    sender_args sargss[num_threads];
    for (int th=0; th<num_threads; th++) {
        sargss[th] = { &some_data[th * size_per_thread], size_per_thread };
        sender_threads[th] = SoftXMT_spawn( &sending_task, &sargss[th] );
    }

    // start the rate test by joining on children threads
    uint64_t start = 0, end = 0;
    std::cout << rank << " starting" << std::endl;
    rdtscll( start );
    for (int th=0; th<num_threads; th++) {
        std::cout << current_thread->id << " will join on " << sender_threads[th]->id << std::endl;
        SoftXMT_join( sender_threads[th] );
        std::cout << current_thread->id << " has finished join on " << sender_threads[th]->id << std::endl;
    }
    rdtscll( end );

    std::cout << "random value " << some_data[rand()%data_size] << std::endl;
    std::cout << "start " << start << std::endl;
    std::cout << "end   " << end << std::endl;


    uint64_t runtime_ns = end - start;
    double rateM = ((double)data_size) / (runtime_ns/BILLION);
    std::cout << "num threads " << num_threads << ", runtime " << runtime_ns << ", rate " << rateM/MILLION << " Mread" << std::endl;

    SoftXMT_signal_done();
}

int main(int argc, char* argv[]) {
  std::cout << "proc " << &some_data << std::endl;

  SoftXMT_init( &argc, &argv );

  SoftXMT_activate();

  user_main_args uargs = { argc, argv };
  SoftXMT_run_user_main( &user_main,  &uargs );

  SoftXMT_finish( 0 );
}
