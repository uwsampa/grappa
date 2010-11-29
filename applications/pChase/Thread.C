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


#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#include "Thread.h"

#include "Lock.h"

Lock Thread::_global_lock;
int  Thread::count = 0;

Thread::Thread()
{
    Thread::global_lock();
	this->id = Thread::count;
	Thread::count += 1;
    Thread::global_unlock();
}

Thread::~Thread()
{
}

int
Thread::start()
{
    return pthread_create(&this->thread, NULL, Thread::start_routine, this); 
}

void*
Thread::start_routine(void* p)
{
    ((Thread*)p)->run();

    return NULL;
}

void
Thread::exit()
{
    pthread_exit(NULL);
}

int
Thread::wait()
{
    pthread_join(this->thread, NULL);

    return 0;
}

void
Thread::lock()
{
    this->object_lock.lock();
}

void
Thread::unlock()
{
    this->object_lock.unlock();
}

void
Thread::global_lock()
{
    Thread::_global_lock.lock();
}

void
Thread::global_unlock()
{
    Thread::_global_lock.unlock();
}
