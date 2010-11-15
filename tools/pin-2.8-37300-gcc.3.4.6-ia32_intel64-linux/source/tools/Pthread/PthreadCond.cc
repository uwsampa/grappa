#include "PthreadCond.h"

using namespace PinPthread;

/* --------------------------------------------------------------------------- */
/* HasMoreWaiters:                                                             */
/* determine whether there are threads waiting on the given condition variable */
/* --------------------------------------------------------------------------- */

bool PthreadCondManager::HasMoreWaiters(pthread_cond_t* cond) 
{
    return (waiters.find(cond) != waiters.end());
}

/* --------------------------------------------------------------------------- */
/* AddWaiter:                                                                  */
/* force the given thread to wait on the given condition variable              */
/* --------------------------------------------------------------------------- */

void PthreadCondManager::AddWaiter(pthread_cond_t* cond, pthread_t thread,
                                   pthread_mutex_t* mutex) 
{
    pthreadcond_queue_t::iterator condptr = waiters.find(cond);
    if (condptr == waiters.end()) 
    {
        waiters[cond] = new PthreadWaiters(thread, mutex);
    }
    else 
    {
        (condptr->second)->PushWaiter(thread, mutex);
    }
}

/* --------------------------------------------------------------------------- */
/* RemoveWaiter:                                                               */
/* release the next thread waiting on the given condition variable             */
/* --------------------------------------------------------------------------- */

void PthreadCondManager::RemoveWaiter(pthread_cond_t* cond, pthread_t* thread,
                                      pthread_mutex_t** mutex) 
{
    pthreadcond_queue_t::iterator condptr = waiters.find(cond);
    ASSERTX(condptr != waiters.end());
    (condptr->second)->PopWaiter(thread, mutex);
    if ((condptr->second)->IsEmpty())
    {
        waiters.erase(condptr);
    }
}

/* --------------------------------------------------------------------------- */
/* PthreadWaiters Functions:                                                   */
/* --------------------------------------------------------------------------- */

PthreadWaiters::PthreadWaiters(pthread_t thread1, pthread_mutex_t* mutex1) 
{
    PushWaiter(thread1, mutex1);
}

PthreadWaiters::~PthreadWaiters() {}

void PthreadWaiters::PushWaiter(pthread_t thread, pthread_mutex_t* mutex) 
{
    pthread_waitstate_t waiter;
    waiter.first = thread;
    waiter.second = mutex;
    waiters.push_back(waiter);
}

void PthreadWaiters::PopWaiter(pthread_t* thread, pthread_mutex_t** mutex) 
{
    ASSERTX(!waiters.empty());
    pthread_waitstate_t waiter = waiters.front();
    *thread = waiter.first;
    *mutex = waiter.second;
    waiters.erase(waiters.begin());
}

bool PthreadWaiters::IsEmpty() 
{
    return (waiters.empty());
}

/* --------------------------------------------------------------------------- */
/* pthread_cond_init:                                                          */
/* initializes the condition                                                   */
/* as in the linuxthreads implementation, do nothing                           */
/* --------------------------------------------------------------------------- */

int PthreadCond::pthread_cond_init(pthread_cond_t* cond, pthread_condattr_t* attr) 
{
    return 0;
}
