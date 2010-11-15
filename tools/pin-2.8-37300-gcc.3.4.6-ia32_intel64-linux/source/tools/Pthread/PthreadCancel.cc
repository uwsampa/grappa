#include "PthreadCancel.h"

using namespace PinPthread;

/* --------------------------------------------------------------------------- */
/* AddThread:                                                                  */
/* add a new thread to keep track of its cancellation state                    */
/* --------------------------------------------------------------------------- */

void PthreadCancelManager::AddThread(pthread_t thread)
{
    ASSERTX(pthreads.find(thread) == pthreads.end());
    pthreads[thread] = new PthreadCancel();
}

/* --------------------------------------------------------------------------- */
/* KillThread:                                                                 */
/* remove resources for the terminated thread                                  */
/* --------------------------------------------------------------------------- */

void PthreadCancelManager::KillThread(pthread_t thread) 
{
    ASSERTX(pthreads.find(thread) != pthreads.end());
    pthreads.erase(thread);
}

/* --------------------------------------------------------------------------- */
/* Cancel:                                                                     */
/* send a cancellation request to the given thread                             */
/* return true if the thread should be immediately terminated, false otherwise */
/* --------------------------------------------------------------------------- */

bool PthreadCancelManager::Cancel(pthread_t thread) 
{
    PthreadCancel* thread_cancel_state = GetThreadCancelState(thread);
    if (thread_cancel_state->cancelstate == PTHREAD_CANCEL_ENABLE) 
    {
        if (thread_cancel_state->canceltype == PTHREAD_CANCEL_ASYNCHRONOUS) 
        {
            return true;
        }
        else  
        {
            ASSERTX(thread_cancel_state->canceltype == PTHREAD_CANCEL_DEFERRED);
            thread_cancel_state->canceled = true;
        }
    }
    return false;
}

/* --------------------------------------------------------------------------- */
/* IsCanceled:                                                                 */
/* determine whether the given thread should be terminated                     */
/* --------------------------------------------------------------------------- */

bool PthreadCancelManager::IsCanceled(pthread_t thread) 
{
    PthreadCancel* thread_cancel_state = GetThreadCancelState(thread);
    return thread_cancel_state->canceled;
}
    
/* --------------------------------------------------------------------------- */
/* SetState:                                                                   */
/* change the cancellation state for the given thread                          */
/* --------------------------------------------------------------------------- */

int PthreadCancelManager::SetState(pthread_t thread, int newstate, int* oldstate) 
{
    if ((newstate != PTHREAD_CANCEL_ENABLE) && (newstate != PTHREAD_CANCEL_DISABLE)) 
    {
        std::cout << "[ERROR] Not a Valid Cancel State: "
                  << dec << newstate << "!\n" << flush;
        return EINVAL;
    }
    else 
    {
        PthreadCancel* thread_cancel_state = GetThreadCancelState(thread);
        if (oldstate != NULL) 
        {
            *oldstate = thread_cancel_state->cancelstate;
        }
        thread_cancel_state->cancelstate = newstate;
        return 0;
    }
}

/* --------------------------------------------------------------------------- */
/* SetType:                                                                   */
/* change the cancellation type for the given thread                          */
/* --------------------------------------------------------------------------- */

int PthreadCancelManager::SetType(pthread_t thread, int newtype, int* oldtype) 
{
    if ((newtype != PTHREAD_CANCEL_ASYNCHRONOUS) && (newtype != PTHREAD_CANCEL_DEFERRED)) 
    {
        std::cout << "[ERROR] Not a Valid Cancel Type: "
                  << dec << newtype << "!\n" << flush;
        return EINVAL;
    }
    else 
    {
        PthreadCancel* thread_cancel_state = GetThreadCancelState(thread);
        if (oldtype != NULL) 
        {
            *oldtype = thread_cancel_state->canceltype;
        }
        thread_cancel_state->canceltype = newtype;
        return 0;
    }
}

/* --------------------------------------------------------------------------- */
/* PthreadCancel Constructor and Destructor:                                   */
/* --------------------------------------------------------------------------- */

PthreadCancel::PthreadCancel() :
    canceled(false),
    cancelstate(PTHREAD_CANCEL_ENABLE),
    canceltype(PTHREAD_CANCEL_DEFERRED) {}

PthreadCancel::~PthreadCancel()         {}

/* --------------------------------------------------------------------------- */
/* PthreadCancelManager Helper Function(s):                                    */
/* --------------------------------------------------------------------------- */

PthreadCancel* PthreadCancelManager::GetThreadCancelState(pthread_t thread) 
{
    pthreadcancel_queue_t::iterator threadptr = pthreads.find(thread);
    ASSERTX(threadptr != pthreads.end());
    return threadptr->second;
}

