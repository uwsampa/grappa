// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#pragma once

#include "Worker.hpp"

namespace Grappa {

class Scheduler {
    private:
        void dummy() {};
    public:
       virtual Worker * get_current_thread() = 0;
       virtual void assignTid( Worker * thr ) = 0;

       virtual void ready( Worker * thr ) = 0;
       virtual void periodic( Worker * thr ) = 0;
       virtual void run( ) = 0;
       
       virtual bool thread_yield( ) = 0;
       virtual void thread_suspend( ) = 0;
       virtual void thread_wake( Worker * next ) = 0;
       virtual void thread_yield_wake( Worker * next ) = 0;
       virtual void thread_suspend_wake( Worker * next ) = 0;

       virtual Worker * thread_wait( void **result ) = 0;
       virtual void thread_on_exit( ) = 0;
};

}