#ifndef _SCHEDULER_HPP_
#define _SCHEDULER_HPP_

#include "Thread.hpp"

class Scheduler {
    private:
        void dummy() {};
    public:
       virtual thread * get_current_thread() = 0;
       virtual void assignTid( thread * thr ) = 0;

       virtual void ready( thread* thr ) = 0;
       virtual void periodic( thread* thr ) = 0;
       virtual void run( ) = 0;
       
       virtual bool thread_yield( ) = 0;
       virtual void thread_suspend( ) = 0;
       virtual void thread_wake( thread * next ) = 0;
       virtual void thread_yield_wake( thread * next ) = 0;
       virtual void thread_suspend_wake( thread * next ) = 0;
       virtual void thread_join( thread* wait_on ) = 0;

       virtual thread * thread_wait( void **result ) = 0;
       virtual void thread_on_exit( ) = 0;
};

#endif
