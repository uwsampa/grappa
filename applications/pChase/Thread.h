/*******************************************************************************
 * Copyright (c) 2006 International Business Machines Corporation.             *
 * All rights reserved. This program and the accompanying materials            *
 * are made available under the terms of the Common Public License v1.0        *
 * which accompanies this distribution, and is available at                    *
 * http://www.opensource.org/licenses/cpl1.0.php                               *
 *                                                                             *
 * Contributors:                                                               *
 *    Douglas M. Pase - initial API and implementation                         *
 *******************************************************************************/


#if !defined(Thread_h)
#define Thread_h

#include <pthread.h>

#include "Lock.h"

class Thread {
public:
    Thread();
    ~Thread();

    virtual int run() = 0;

    int start();
    int wait();
    int thread_count() { return Thread::count; }
    int thread_id() { return id; }

    static void exit();

protected:
    void lock();
    void unlock();
    static void global_lock();
    static void global_unlock();

private:
    static void* start_routine(void *);
    static Lock  _global_lock;

    Lock  object_lock;

    pthread_t thread;

    static int count;
    int id;
    int lock_obj;
};

#endif
