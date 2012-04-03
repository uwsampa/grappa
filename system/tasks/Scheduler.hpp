#ifndef _SCHEDULER_HPP_
#define _SCHEDULER_HPP_

#include "Thread.hpp"

class Scheduler {
    private:
        void dummy() {};
    public:
       virtual Thread * get_current_thread() = 0;
       virtual void assignTid( Thread * thr ) = 0;

       virtual void ready( Thread * thr ) = 0;
       virtual void periodic( Thread * thr ) = 0;
       virtual void run( ) = 0;
       
       virtual bool thread_yield( ) = 0;
       virtual void thread_suspend( ) = 0;
       virtual void thread_wake( Thread * next ) = 0;
       virtual void thread_yield_wake( Thread * next ) = 0;
       virtual void thread_suspend_wake( Thread * next ) = 0;
       virtual void thread_join( Thread * wait_on ) = 0;

       virtual Thread * thread_wait( void **result ) = 0;
       virtual void thread_on_exit( ) = 0;
};

#endif
