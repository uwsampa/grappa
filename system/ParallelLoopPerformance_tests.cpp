#include <boost/test/unit_test.hpp>

#include "SoftXMT.hpp"
#include "Delegate.hpp"
#include "Tasking.hpp"
#include "ParallelLoop.hpp"
#include "tasks/DictOut.hpp"
#include "Future.hpp"
#include "ForkJoin.hpp"

#include <TAU.h>

#include <sys/time.h>
double wctime() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (tv.tv_sec + 1E-6 * tv.tv_usec);
}

DEFINE_bool( fdonly, false, "Only run futures impl, delegate iter");
DEFINE_int64( iters, 23, "number of parallel loop iterations");

BOOST_AUTO_TEST_SUITE( ParallelLoopPerformance_tests );

int64_t ind_local_count = 0;
int64_t update_me = 0;

const int64_t length1 = 1024;

struct array_args {
    int64_t * array;
};

///
/// Parallel loop bodies
///
void simple_iter( int64_t i, array_args * a ) {
    ind_local_count+=i;
}

void mod_iter( int64_t i, int64_t * ignore ) {
    ind_local_count+= i % SoftXMT_nodes();
}

void delegate_iter( int64_t i, int64_t * ignore ) {
    SoftXMT_delegate_write_word( make_global( &update_me, i % SoftXMT_nodes() ), i );
}

void half_iter( int64_t i, int64_t * ignore ) {
    if (i % 2 == 0) {
        ind_local_count+=i;
    } else {
        SoftXMT_delegate_write_word( make_global( &update_me, (i/2) % SoftXMT_nodes() ), i );
    }
}

void yield_iter( int64_t i, int64_t * ignore ) {
    ind_local_count+=i;
    SoftXMT_yield();
}

//void yield_next_iter( int64_t i, int64_t * ignore ) {
//    ind_local_count+=i;
//    SoftXMT_yield_next();
//}

///
/// ForkJoin loop bodies
///
struct func_simple : public ForkJoinIteration {
    array_args a;
    void operator()(int64_t i) const {
        ind_local_count+=i;
    }
};

struct func_mod : public ForkJoinIteration {
    int64_t * ignore;
    void operator()(int64_t i) const {
        ind_local_count+= i % SoftXMT_nodes();
    }
};

struct func_delegate : public ForkJoinIteration {
    int64_t * ignore;
    void operator()(int64_t i) const {
        SoftXMT_delegate_write_word( make_global( &update_me, i % SoftXMT_nodes() ), i );
    }
};

struct func_half : public ForkJoinIteration {
    int64_t * ignore;
    void operator()(int64_t i) const {
        if (i % 2 == 0) {
            ind_local_count+=i;
        } else {
            SoftXMT_delegate_write_word( make_global( &update_me, (i/2) % SoftXMT_nodes() ), i );
        }
    }
};

struct func_yield : public ForkJoinIteration {
    int64_t * ignore;
    void operator()(int64_t i) const {
        ind_local_count+=i;
        SoftXMT_yield();
    }
};
//struct func_yield_next : public ForkJoinIteration {
//    int64_t * ignore;
//    void operator()(int64_t i) const {
//        ind_local_count+=i;
//        SoftXMT_yield_next();
//    }
//};

///
/// ForkJoin dump stats
///
LOOP_FUNCTION(futures_reset_stats_func,nid) {
  futures_reset_stats();
}
LOOP_FUNCTION(futures_dump_stats_func,nid) {
  futures_dump_stats();
}
void future_dump_stats_all_nodes() {
  futures_dump_stats_func f;
  fork_join_custom(&f);
}
void futures_reset_stats_all_nodes() {
  futures_reset_stats_func f;
  fork_join_custom(&f);
}

LOOP_FUNCTION(tau_disable_func,nid) {
  TAU_DISABLE_INSTRUMENTATION();
}
LOOP_FUNCTION(tau_enable_func,nid) {
  TAU_ENABLE_INSTRUMENTATION();
}
void tau_enable_instrumentation_all() {
    tau_enable_func f;
    fork_join_custom(&f);
}
void tau_disable_instrumentation_all() {
    tau_disable_func f;
    fork_join_custom(&f);
}

LOOP_FUNCTION(tau_db_dump_func,nid) {
  //TAU_DB_DUMP();
  dump_all_task_profiles();
}
LOOP_FUNCTION(tau_db_purge_func,nid) {
  TAU_DB_PURGE();
}
void tau_db_dump_all() {
    tau_db_dump_func f;
    fork_join_custom(&f);
}
void tau_db_purge_all() {
    tau_db_purge_func f;
    fork_join_custom(&f);
}


#define FUTURE_DUMP_ON 0
#if FUTURE_DUMP_ON
#define FUTURE_DUMP future_dump_stats_all_nodes()
#else
#define FUTURE_DUMP (0<0)?0:0
#endif

///
/// User main
///

struct user_main_args {
};

void user_main( user_main_args * args ) 
{
    //tau_disable_instrumentation_all();
    
    int64_t iters = 1 << FLAGS_iters;
    
    DictOut d;
    d.add("iterations", iters);

    int64_t * array1 = new int64_t[length1];
    array_args aa = { array1 };
   
   if (!FLAGS_fdonly) {
    BOOST_MESSAGE( "Cache warm" );
    {
        futures_reset_stats_all_nodes();
        
        // a[i] = i
        double start = wctime(); 
        parallel_loop_implFuture( 0, iters, &simple_iter, aa );
        double end = wctime();

        double runtime = end - start;
        BOOST_MESSAGE( "ca{runtime: " << runtime << ", rate: " << ((double)iters)/runtime << ", iterations: " << iters << "}" );
        FUTURE_DUMP;
        
        d.add("runtime_cacheWarm", runtime);
    }
    
    BOOST_MESSAGE( "Running futures -- simple iter" );
    {
        futures_reset_stats_all_nodes();

        // a[i] = i
        double start = wctime(); 
        parallel_loop_implFuture( 0, iters, &simple_iter, aa );
        double end = wctime();

        double runtime = end - start;
        BOOST_MESSAGE( "fs{runtime: " << runtime << ", rate: " << ((double)iters)/runtime << ", iterations: " << iters << "}" );
        FUTURE_DUMP;
        
        d.add("runtime_simpIter", runtime);
    }
    
    BOOST_MESSAGE( "Running futures -- mod iter" );
    {
        futures_reset_stats_all_nodes();

        // a[i] = i
        double start = wctime(); 
        parallel_loop_implFuture( 0, iters, &mod_iter, (int64_t)0 );
        double end = wctime();

        double runtime = end - start;
        BOOST_MESSAGE( "fm{runtime: " << runtime << ", rate: " << ((double)iters)/runtime << ", iterations: " << iters << "}" );
        FUTURE_DUMP;


        d.add("runtime_modIter", runtime);
    }
   }

    BOOST_MESSAGE( "Running futures -- delegate iter" );
    {
        futures_reset_stats_all_nodes();
        SoftXMT_reset_stats();

        // a[i] = i
        double start, end;
        {
        //tau_enable_instrumentation_all();
        tau_db_purge_all();
        start = wctime(); 
        parallel_loop_implFuture( 0, iters, &delegate_iter, (int64_t)0 );
        end = wctime();
        //tau_disable_instrumentation_all();
        tau_db_dump_all();
        }

        double runtime = end - start;
        BOOST_MESSAGE( "fd{runtime: " << runtime << ", rate: " << ((double)iters)/runtime << ", iterations: " << iters << "}" );
        FUTURE_DUMP;
        SoftXMT_dump_task_series();

        d.add("runtime_delegateIter", runtime);
    }
   
   if (!FLAGS_fdonly) { 
    BOOST_MESSAGE( "Running futures -- half iter" );
    {
        futures_reset_stats_all_nodes();

        // a[i] = i
        double start = wctime(); 
        parallel_loop_implFuture( 0, iters, &half_iter, (int64_t)0 );
        double end = wctime();

        double runtime = end - start;
        BOOST_MESSAGE( "fh{runtime: " << runtime << ", rate: " << ((double)iters)/runtime << ", iterations: " << iters << "}" );
        FUTURE_DUMP;

        d.add("runtime_halfIter", runtime);
    }
    
//    BOOST_MESSAGE( "Running futures -- yield next iter" );
//    {
//        futures_reset_stats_all_nodes();
//
//        // a[i] = i
//        double start = wctime(); 
//        parallel_loop_implFuture( 0, iters, &yield_next_iter, (int64_t)0 );
//        double end = wctime();
//
//        double runtime = end - start;
//        BOOST_MESSAGE( "fyn{runtime: " << runtime << ", rate: " << ((double)iters)/runtime << ", iterations: " << iters << "}" );
//        FUTURE_DUMP;
//
//        d.add("runtime_yieldNextIter", runtime);
//    }
    
    BOOST_MESSAGE( "Running futures -- yield iter" );
    {
        futures_reset_stats_all_nodes();

        // a[i] = i
        double start = wctime(); 
        parallel_loop_implFuture( 0, iters, &yield_iter, (int64_t)0 );
        double end = wctime();

        double runtime = end - start;
        BOOST_MESSAGE( "fy{runtime: " << runtime << ", rate: " << ((double)iters)/runtime << ", iterations: " << iters << "}" );
        FUTURE_DUMP;

        d.add("runtime_yieldIter", runtime);
    }
    
    /// Semaphore parallel loop
    BOOST_MESSAGE( "Running semaphores -- simple iter" );
    {

        // a[i] = i
        double start = wctime(); 
        parallel_loop_implSemaphore( 0, iters, &simple_iter, aa );
        double end = wctime();

        double runtime = end - start;
        BOOST_MESSAGE( "ss{runtime: " << runtime << ", rate: " << ((double)iters)/runtime << ", iterations: " << iters << "}" );
        
        d.add("runtime_semaphores_simpIter", runtime);
    }
    
    BOOST_MESSAGE( "Running semaphores -- mod iter" );
    {

        // a[i] = i
        double start = wctime(); 
        parallel_loop_implSemaphore( 0, iters, &mod_iter, (int64_t)0 );
        double end = wctime();

        double runtime = end - start;
        BOOST_MESSAGE( "sm{runtime: " << runtime << ", rate: " << ((double)iters)/runtime << ", iterations: " << iters << "}" );


        d.add("runtime_semaphores_modIter", runtime);
    }

    BOOST_MESSAGE( "Running semaphores -- delegate iter" );
    {
        // a[i] = i
        double start = wctime(); 
        parallel_loop_implSemaphore( 0, iters, &delegate_iter, (int64_t)0 );
        double end = wctime();

        double runtime = end - start;
        BOOST_MESSAGE( "sd{runtime: " << runtime << ", rate: " << ((double)iters)/runtime << ", iterations: " << iters << "}" );

        d.add("runtime_semaphores_delegateIter", runtime);
    }
    
    BOOST_MESSAGE( "Running semaphores -- half iter" );
    {
        // a[i] = i
        double start = wctime(); 
        parallel_loop_implSemaphore( 0, iters, &half_iter, (int64_t)0 );
        double end = wctime();

        double runtime = end - start;
        BOOST_MESSAGE( "sh{runtime: " << runtime << ", rate: " << ((double)iters)/runtime << ", iterations: " << iters << "}" );

        d.add("runtime_semaphores_halfIter", runtime);
    }
    
    BOOST_MESSAGE( "Running semaphores -- yield iter" );
    {
        // a[i] = i
        double start = wctime(); 
        parallel_loop_implSemaphore( 0, iters, &yield_iter, (int64_t)0 );
        double end = wctime();

        double runtime = end - start;
        BOOST_MESSAGE( "sy{runtime: " << runtime << ", rate: " << ((double)iters)/runtime << ", iterations: " << iters << "}" );
        
        d.add("runtime_semaphores_yieldIter", runtime);
    }

    /// ForkJoin
//    BOOST_MESSAGE( "Running fork join -- simple iter" );
//    {
//
//        // a[i] = i
//        double start = wctime();
//        func_simple fs;
//        fs.a = aa;
//        fork_join(&fs, 0, iters);
//        double end = wctime();
//
//        double runtime = end - start;
//        BOOST_MESSAGE( "fj_s{runtime: " << runtime << ", rate: " << ((double)iters)/runtime << ", iterations: " << iters << "}" );
//        
//        d.add("runtime_forkjoin_simple", runtime);
//    }
//    
//    BOOST_MESSAGE( "Running fork join -- mod iter" );
//    {
//
//        // a[i] = i
//        double start = wctime();
//        func_mod fm;
//        int64_t ii = 0;
//        fm.ignore = &ii;
//        fork_join(&fm, 0, iters);
//        double end = wctime();
//
//        double runtime = end - start;
//        BOOST_MESSAGE( "fj_m{runtime: " << runtime << ", rate: " << ((double)iters)/runtime << ", iterations: " << iters << "}" );
//        
//        d.add("runtime_forkjoin_mod", runtime);
//    }
//    
//    BOOST_MESSAGE( "Running fork join -- delegate iter" );
//    {
//
//        // a[i] = i
//        double start = wctime();
//        func_delegate fd;
//        int64_t ii = 0;
//        fd.ignore = &ii;
//        fork_join(&fd, 0, iters);
//        double end = wctime();
//
//        double runtime = end - start;
//        BOOST_MESSAGE( "fd_m{runtime: " << runtime << ", rate: " << ((double)iters)/runtime << ", iterations: " << iters << "}" );
//        
//        d.add("runtime_forkjoin_delegate", runtime);
//    }
//    
//    BOOST_MESSAGE( "Running fork join -- half iter" );
//    {
//
//        // a[i] = i
//        double start = wctime();
//        func_half fh;
//        int64_t ii = 0;
//        fh.ignore = &ii;
//        fork_join(&fh, 0, iters);
//        double end = wctime();
//
//        double runtime = end - start;
//        BOOST_MESSAGE( "fh_m{runtime: " << runtime << ", rate: " << ((double)iters)/runtime << ", iterations: " << iters << "}" );
//        
//        d.add("runtime_forkjoin_half", runtime);
//    }
//    
//    BOOST_MESSAGE( "Running fork join -- yield iter" );
//    {
//
//        // a[i] = i
//        double start = wctime();
//        func_yield fy;
//        int64_t ii = 0;
//        fy.ignore = &ii;
//        fork_join(&fy, 0, iters);
//        double end = wctime();
//
//        double runtime = end - start;
//        BOOST_MESSAGE( "fy_m{runtime: " << runtime << ", rate: " << ((double)iters)/runtime << ", iterations: " << iters << "}" );
//        
//        d.add("runtime_forkjoin_yield", runtime);
//    }

//    BOOST_MESSAGE( "Running fork join -- yield_next iter" );
//    {
//
//        // a[i] = i
//        double start = wctime();
//        func_yield_next fyn;
//        int64_t ii = 0;
//        fyn.ignore = &ii;
//        fork_join(&fyn, 0, iters);
//        double end = wctime();
//
//        double runtime = end - start;
//        BOOST_MESSAGE( "fyn_m{runtime: " << runtime << ", rate: " << ((double)iters)/runtime << ", iterations: " << iters << "}" );
//        
//        d.add("runtime_forkjoin_yield_next", runtime);
//    }
    }

    // print out results
    BOOST_MESSAGE( "parloop_perf:" << d.toString( ) );
    
//    BOOST_MESSAGE( "Running single serial spawn" );
//    {
//        // a[i] = i
//        double start = wctime(); 
//        //parallel_loop_implSingleSerial( 0, iters, &ind_array, aa );
//        double end = wctime();
//        
//        double runtime = end - start;
//        BOOST_MESSAGE( "runtime: " << runtime );
//        BOOST_MESSAGE( "{runtime: " << runtime << ", rate: " << ((double)iters)/runtime << "}" );
//
//    }
//
//    BOOST_MESSAGE( "Running single" );
//    {
//        // a[i] = i
//        double start = wctime(); 
////        parallel_loop_implSingle( 0, iters, &ind_array, aa );
//        double end = wctime();
//        
//        double runtime = end - start;
//        BOOST_MESSAGE( "runtime: " << runtime );
//
//    }

    SoftXMT_dump_stats_all_nodes();
    BOOST_MESSAGE( "user main is exiting" );
}



BOOST_AUTO_TEST_CASE( test1 ) {

    SoftXMT_init( &(boost::unit_test::framework::master_test_suite().argc),
            &(boost::unit_test::framework::master_test_suite().argv) );

    SoftXMT_activate();

    user_main_args uargs;

    //TAU_DISABLE_INSTRUMENTATION();
    
    DVLOG(1) << "Spawning user main Thread....";
    SoftXMT_run_user_main( &user_main, &uargs );
    VLOG(5) << "run_user_main returned";
    CHECK( SoftXMT_done() );
    
    //TAU_ENABLE_INSTRUMENTATION();

    SoftXMT_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();

