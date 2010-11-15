#include "PthreadMutex.h"

using namespace PinPthread;

/* --------------------------------------------------------------------------- */
/* pthread_mutex_destroy:                                                      */
/* destroy the given mutex object, i.e. free up resources it might hold        */
/* --------------------------------------------------------------------------- */

int PthreadMutex::pthread_mutex_destroy(pthread_mutex_t* mutex) 
{
    /* do nothing except check that the mutex is unlocked, as in LinuxThreads */
    if (IsLocked(mutex)) 
    {
        std::cout << "[ERROR] Destroying a Locked Mutex: " << hex << mutex << "\n" << flush;
        return EBUSY;
    }
    else 
    {
        return 0;
    }
}

/* --------------------------------------------------------------------------- */
/* pthread_mutex_init:                                                         */
/* --------------------------------------------------------------------------- */

int PthreadMutex::pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* mutexattr) 
{
    ASSERTX(mutex != NULL);
    if (mutexattr != NULL) 
    {
        PthreadMutexAttr::pthread_mutexattr_getkind(mutexattr, &(mutex->__m_kind));
    }
    else 
    {
        mutex->__m_kind = (PthreadMutexAttr::PTHREAD_MUTEXATTR_DEFAULT()).__mutexkind;
    }
    mutex->__m_count = 0;
    mutex->__m_reserved = -1;
    return 0;
}

/* --------------------------------------------------------------------------- */
/* Lock:                                                                       */
/* implement pthread_mutex_lock and pthread_mutex_trylock                      */
/* return 0 if it succeeds in locking the mutex                                */
/* else return the error code for trylock or a nonzero value for lock          */
/* --------------------------------------------------------------------------- */

int PthreadMutexManager::Lock(pthread_t thread, pthread_mutex_t* mutex, bool trylock) 
{
    if (!PthreadMutex::IsLocked(mutex))                /* unlocked: the thread gets to lock the mutex */
    {
        PthreadMutex::Lock(mutex, thread);
        return 0;
    }
    else if (PthreadMutex::GetOwner(mutex) == thread)  /* locked: the thread is trying to relock the same mutex */
    {
        switch(PthreadMutex::GetKind(mutex))
        {
          case PTHREAD_MUTEX_FAST_NP:
          case PTHREAD_MUTEX_ERRORCHECK_NP:
            std::cout << "[ERROR] DEADLOCK - Thread " << dec << thread 
                      << " is Locking This Lock Again: " << hex << mutex << "\n" << flush;
            if (!trylock) 
            {
                BlockThread(thread, mutex);
            }
            return EDEADLK;
          case PTHREAD_MUTEX_RECURSIVE_NP:
#if VERBOSE
            std::cout << "Thread " << dec << thread
                      << " is Locking This Lock Again: " << hex << mutex << "\n" << flush;
#endif
            PthreadMutex::LockAgain(mutex);
            break;
          default:
            ASSERTX(0);
        }
        return 0;
    }
    else                                               /* locked: the thread spins or returns EBUSY for trylock */
    {
#if VERBOSE
        std::cout << "But Lock " << hex << mutex << " is Locked by Thread "
                  << dec << PthreadMutex::GetOwner(mutex) << "\n" << flush;
#endif
        if (!trylock) 
        {
            BlockThread(thread, mutex);
        }
        return EBUSY;
    }
}

/* --------------------------------------------------------------------------- */
/* Unlock:                                                                     */
/* implement pthread_mutex_unlock                                              */
/* if this unblocks a thread spinning on the given mutex,                      */
/* return true and the id of the unblocked thread; else return false           */
/* return the error return valud for pthread_mutex_unlock through int* error   */
/* --------------------------------------------------------------------------- */

bool PthreadMutexManager::Unlock(pthread_t thread, pthread_mutex_t* mutex,
                                 pthread_t* unblocked, int* error) 
{
    if ((PthreadMutex::GetKind(mutex) == PTHREAD_MUTEX_ERRORCHECK_NP) &&
        (!PthreadMutex::IsLocked(mutex) || (PthreadMutex::GetOwner(mutex) != thread)))
    {
#if VERBOSE
        if (!PthreadMutex::IsLocked(mutex)) 
        {
            std::cout << "[ERROR] Mutex " << hex << mutex << " is Not Locked!\n" << flush;
        }
        else 
        {
            std::cout << "[ERROR] Mutex " << hex << mutex << " is Owned By Thread "
                      << dec << thread << "!\n" << flush;
        }
#endif
        *error = EPERM;
        return false;
    }
    else 
    {
        *error = 0;
        if (PthreadMutex::GetKind(mutex) == PTHREAD_MUTEX_RECURSIVE_NP) 
        {
            PthreadMutex::UnlockAgain(mutex);
            if (PthreadMutex::IsLocked(mutex))
            {
                return false;
            }
        }
        else 
        {
            PthreadMutex::Unlock(mutex);
        }
        return UnblockThread(mutex, unblocked);
    }
}

/* --------------------------------------------------------------------------- */
/* BlockThread:                                                                */
/* the given thread spins on the given mutex                                   */
/* --------------------------------------------------------------------------- */

void PthreadMutexManager::BlockThread(pthread_t thread, pthread_mutex_t* mutex) 
{
    pthreadspinning_queue_t::iterator mutexptr = spinning.find(mutex);
    if (mutexptr == spinning.end()) 
    {
        spinning[mutex] = new PthreadsSpinning(thread);
    }
    else 
    {
        PthreadsSpinning* thread_queue = spinning[mutex];
        thread_queue->PushThread(thread);
    }
}

/* --------------------------------------------------------------------------- */
/* UnblockThread:                                                              */
/* if the unlocking of the given mutex unblocks a thread spinning on it,       */
/* allow the unblocked thread to lock the given mutex, and                     */
/* return true and the id of the unblocked thread; else return false           */
/* --------------------------------------------------------------------------- */

bool PthreadMutexManager::UnblockThread(pthread_mutex_t* mutex, pthread_t* unblocked) 
{
    pthreadspinning_queue_t::iterator mutexptr = spinning.find(mutex);
    if (mutexptr == spinning.end()) 
    {
        return false;
    }
    else 
    {
        *unblocked = (mutexptr->second)->PopThread();
        if ((mutexptr->second)->IsEmpty()) 
        {
            spinning.erase(mutexptr);
        }
#if VERBOSE
        std::cout << "Spinning Thread " << dec << *unblocked << " Locks Mutex "
                  << hex << mutex << "\n" << flush;
#endif
        Lock(*unblocked, mutex, false);
        return true;
    }
}

/* --------------------------------------------------------------------------- */
/* PthreadsSpinning Functions:                                                 */
/* --------------------------------------------------------------------------- */

PthreadsSpinning::PthreadsSpinning(pthread_t thread1) 
{
    PushThread(thread1);
}

PthreadsSpinning::~PthreadsSpinning() {}

void PthreadsSpinning::PushThread(pthread_t thread) 
{
    spinning.push_back(thread);
}

pthread_t PthreadsSpinning::PopThread() 
{
    ASSERTX(!spinning.empty());
    pthread_t thread = spinning.front();
    spinning.erase(spinning.begin());
    return thread;
}

bool PthreadsSpinning::IsEmpty() 
{
    return spinning.empty();
}

/* --------------------------------------------------------------------------- */
/* PthreadMutex Helper Functions:                                              */
/* --------------------------------------------------------------------------- */

bool PthreadMutex::IsLocked(pthread_mutex_t* mutex) 
{
    ASSERTX(mutex != NULL);
    ASSERTX((mutex->__m_count) >= 0);
    return ((mutex->__m_count) > 0);
}

void PthreadMutex::Lock(pthread_mutex_t* mutex, pthread_t owner) 
{
    ASSERTX(mutex != NULL);
    mutex->__m_count = 1;
    mutex->__m_reserved = (int)owner;
}

void PthreadMutex::LockAgain(pthread_mutex_t* mutex) 
{
    ASSERTX(mutex != NULL);
    ASSERTX(mutex->__m_kind == PTHREAD_MUTEX_RECURSIVE_NP);
    ASSERTX(mutex->__m_count >= 1);
    (mutex->__m_count)++;
}

void PthreadMutex::Unlock(pthread_mutex_t* mutex) 
{
    ASSERTX(mutex != NULL);
    mutex->__m_count = 0;
}

void PthreadMutex::UnlockAgain(pthread_mutex_t* mutex) 
{
    ASSERTX(mutex != NULL);
    ASSERTX(mutex->__m_kind == PTHREAD_MUTEX_RECURSIVE_NP);
    ASSERTX(mutex->__m_count >= 1);
    (mutex->__m_count)--;
}

pthread_t PthreadMutex::GetOwner(pthread_mutex_t* mutex) 
{
    ASSERTX(mutex != NULL);
    return (pthread_t)mutex->__m_reserved;
}

int PthreadMutex::GetKind(pthread_mutex_t* mutex) 
{
    ASSERTX(mutex != NULL);
    if (mutex->__m_kind == PTHREAD_MUTEX_DEFAULT) 
    {
        return PTHREAD_MUTEX_FAST_NP;
    }
    else 
    {
        ASSERTX((mutex->__m_kind == PTHREAD_MUTEX_FAST_NP) ||
                (mutex->__m_kind == PTHREAD_MUTEX_RECURSIVE_NP) ||
                (mutex->__m_kind == PTHREAD_MUTEX_ERRORCHECK_NP));
        return mutex->__m_kind;
    }
}

