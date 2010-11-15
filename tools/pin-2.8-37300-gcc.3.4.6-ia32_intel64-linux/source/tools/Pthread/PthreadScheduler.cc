#include "PthreadScheduler.h"

using namespace PinPthread;

/* --------------------------------------------------------------------------- */
/* PthreadScheduler Constructor and Destructor                                 */
/* --------------------------------------------------------------------------- */

PthreadScheduler::PthreadScheduler() :
    nactive(0)                        {}

PthreadScheduler::~PthreadScheduler() {}

/* --------------------------------------------------------------------------- */
/* AddThread:                                                                  */
/* add an active thread to the queue                                           */
/* --------------------------------------------------------------------------- */

void PthreadScheduler::AddThread(pthread_t thread, pthread_attr_t* attr,
                                 CONTEXT* startctxt,
                                 ADDRINT func, ADDRINT arg)
{
    ASSERTX(pthreads.find(thread) == pthreads.end());
    pthreads[thread] = new Pthread(attr, startctxt, func, arg);
    if (pthreads.size() == 1) 
    {
        current = pthreads.begin();
    }
    nactive++;
}

/* --------------------------------------------------------------------------- */
/* KillThread:                                                                 */
/* destroy the given thread                                                    */
/* --------------------------------------------------------------------------- */

void PthreadScheduler::KillThread(pthread_t thread) 
{
    if (thread == GetCurrentThread()) 
    {
        ASSERTX(IsActive(thread));
        FixupCurrentPtr();
    }
    pthreads.erase(thread);
    nactive--;
}

/* --------------------------------------------------------------------------- */
/* SwitchThreads:                                                              */
/* schedule a new thread, or run a new thread upon the current                 */
/* thread's termination                                                        */
/* --------------------------------------------------------------------------- */

void PthreadScheduler::SwitchThreads(CHECKPOINT* chkpt, bool mustswitch) 
{
    if (pthreads.empty()) 
    {
#if VERBOSE
        std::cout << "No more threads to run - exit program!\n" << flush;
#endif
        std::exit(0);
    }
    else if ((nactive > 1)  || mustswitch)
    {
        if (chkpt != NULL) 
        {
#if VERYVERYVERBOSE
            std::cout << "Save Thread " << dec << GetCurrentThread() << "\n" << flush;
#endif
            PIN_SaveCheckpoint(chkpt, GetCurrentCheckpoint());
        }
        AdvanceCurrentPtr();
        if (HasStarted(current))
        {
#if VERYVERYVERBOSE
            std::cout << "Run Thread " << dec << GetCurrentThread() << "\n" << flush;
#endif
            PIN_Resume(GetCurrentCheckpoint());
        }
        else 
        {
#if VERBOSE
            std::cout << "Start Thread " << dec << GetCurrentThread() << "\n" << flush;
#endif
            StartThread(current);
            PIN_ExecuteAt(GetCurrentStartCtxt());
        }
    }
}

/* --------------------------------------------------------------------------- */
/* BlockThread:                                                                */
/* deschedule the given thread                                                 */
/* --------------------------------------------------------------------------- */

void PthreadScheduler::BlockThread(pthread_t thread) 
{
    ASSERTX(IsActive(thread));
    SetActiveState(thread, false);
    nactive--;
    ASSERT(nactive > 0, "[ERROR] Deadlocked!\n");
}

/* --------------------------------------------------------------------------- */
/* UnblockThread:                                                              */
/* enable the given thread to be scheduled again                               */
/* --------------------------------------------------------------------------- */

void PthreadScheduler::UnblockThread(pthread_t thread) 
{
    ASSERTX(!IsActive(thread));
    SetActiveState(thread, true);
    nactive++;
}

/* --------------------------------------------------------------------------- */
/* GetCurrentThread:                                                           */
/* return the id of the current thread running                                 */
/* --------------------------------------------------------------------------- */

pthread_t PthreadScheduler::GetCurrentThread() 
{
    return current->first;
}

/* --------------------------------------------------------------------------- */
/* IsThreadValid:                                                              */
/* determine whether the given thread is valid (active or inactive)            */
/* --------------------------------------------------------------------------- */

bool PthreadScheduler::IsThreadValid(pthread_t thread) 
{
    return (pthreads.find(thread) != pthreads.end());
}

/* --------------------------------------------------------------------------- */
/* GetAttr:                                                                    */
/* return the given thread's attribute fields relevant to the scheduler        */
/* --------------------------------------------------------------------------- */

void PthreadScheduler::GetAttr(pthread_t thread, pthread_attr_t* attr) 
{
    pthread_queue_t::iterator threadptr = pthreads.find(thread);
    ADDRINT stacksize = (threadptr->second)->stacksize;
    ADDRINT* stack = (threadptr->second)->stack;
    if (stack == NULL) 
    {
        PthreadAttr::pthread_attr_setstack(attr, (void*)0xbfff0000, 0x10000);
    }
    else 
    {
        PthreadAttr::pthread_attr_setstack(attr, (void*)stack, stacksize);
    }
}

/* --------------------------------------------------------------------------- */
/* GetNumActiveThreads:                                                        */
/* return the number of currently active threads                               */
/* --------------------------------------------------------------------------- */

UINT32 PthreadScheduler::GetNumActiveThreads() 
{
    return nactive;
}

/* --------------------------------------------------------------------------- */
/* Scheduling Functions:                                                       */
/* --------------------------------------------------------------------------- */

void PthreadScheduler::SetCurrentPtr(pthread_t thread) 
{
    current = GetThreadPtr(thread);
}

void PthreadScheduler::AdvanceCurrentPtr() 
{
    do 
    {
        current++;
        if (current == pthreads.end()) 
        {
            current = pthreads.begin();
        }
    } while (!IsActive(current));
}

void PthreadScheduler::FixupCurrentPtr() 
{
    if (current == pthreads.begin()) 
    {
        current = --(pthreads.end());
    }
    else 
    {
        current--;
    }
}

/* --------------------------------------------------------------------------- */
/* Pthread Constructor and Destructor:                                         */
/* --------------------------------------------------------------------------- */

Pthread::Pthread(pthread_attr_t* attr, CONTEXT* _startctxt,
                 ADDRINT func, ADDRINT arg) :
    active(true)
{
    if (_startctxt != NULL)   // new threads
    {
        started = false;
        stacksize = 0x100000;
        if (((stacksize / sizeof(ADDRINT)) % 2) == 0)       // align stack
        {
            stacksize += sizeof(ADDRINT);
        }
        stack = (ADDRINT*)PIN_RawMmap(0, stacksize,
                                      PROT_READ | PROT_WRITE | PROT_EXEC,
                                      MAP_PRIVATE | MAP_ANON,
                                      -1, 0);
        ASSERTX(stack != MAP_FAILED);
        mprotect(stack, sizeof(ADDRINT), PROT_NONE);        // delineate top of stack
        ADDRINT* sp = &(stack[stacksize/sizeof(ADDRINT) - 1]);
        ASSERTX(((ADDRINT)sp & 0x7) == 0);
        *(sp--) = arg;
        *(sp--) = func;
        PIN_SaveContext(_startctxt, &startctxt);
        PIN_SetContextReg(&startctxt, REG_STACK_PTR, (ADDRINT)sp);
        PIN_SetContextReg(&startctxt, REG_INST_PTR, (ADDRINT)StartThreadFunc);
    }
    else                      // initial thread
    {
        stack = NULL;
        stacksize = 0;
        started = true;
    }
}

Pthread::~Pthread() 
{
    munmap(stack, stacksize);
}

/* --------------------------------------------------------------------------- */
/* Functions for Manipulating STL Structure(s):                                */
/* --------------------------------------------------------------------------- */

pthread_queue_t::iterator PthreadScheduler::GetThreadPtr(pthread_t thread) 
{
    pthread_queue_t::iterator threadptr = pthreads.find(thread);
    ASSERTX(threadptr != pthreads.end());
    return threadptr;
}

bool PthreadScheduler::IsActive(pthread_t thread) 
{
    return IsActive(GetThreadPtr(thread));
}

bool PthreadScheduler::IsActive(pthread_queue_t::iterator threadptr) 
{
    return ((threadptr->second)->active);
}

void PthreadScheduler::SetActiveState(pthread_t thread, bool active) 
{
    pthread_queue_t::iterator threadptr = GetThreadPtr(thread);
    (threadptr->second)->active = active;
}

bool PthreadScheduler::HasStarted(pthread_queue_t::iterator threadptr) 
{
    return ((threadptr->second)->started);
}

void PthreadScheduler::StartThread(pthread_queue_t::iterator threadptr) 
{
    (threadptr->second)->started = true;
}

CHECKPOINT* PthreadScheduler::GetCurrentCheckpoint() 
{
    return (&((current->second)->chkpt));
}

CONTEXT* PthreadScheduler::GetCurrentStartCtxt() 
{
    return (&((current->second)->startctxt));
}


