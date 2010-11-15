#include "PthreadJoin.h"

using namespace PinPthread;

/* --------------------------------------------------------------------------- */
/* AddThread:                                                                  */
/* if the new thread is joinable, add to the list of joinable threads          */
/* --------------------------------------------------------------------------- */

void PthreadJoinManager::AddThread(pthread_t thread, pthread_attr_t* attr) 
{
    int detachstate;
    PthreadAttr::pthread_attr_getdetachstate(attr, &detachstate);
    if (detachstate == PTHREAD_CREATE_JOINABLE) 
    {
        InsertJoinee(thread);
    }
}

/* --------------------------------------------------------------------------- */
/* KillThread:                                                                 */
/* for joinable threads...                                                     */
/* if it is being joined, activate and return the joiner thread                */
/* else save the return value and return false                                 */
/* --------------------------------------------------------------------------- */

bool PthreadJoinManager::KillThread(pthread_t thread, void* retval, pthread_t* joinerptr) 
{
    if (IsJoinable(thread)) 
    {
        if (IsJoined(thread)) 
        {
            pthread_t joiner = GetJoiner(thread);
            ASSERTX(GetJoinee(joiner) == thread);
            void** retvalptr = GetRetvalptr(joiner);
            if (retvalptr != NULL) 
            {
                *retvalptr = retval;
            }
            EraseJoiner(joiner);
            EraseJoinee(thread);
            *joinerptr = joiner;
            return true;
        }
        else 
        {
            SetRetval(thread, retval);
        }
    }
    return false;
}

/* --------------------------------------------------------------------------- */
/* DetachThread:                                                               */
/* detach the given thread                                                     */
/* --------------------------------------------------------------------------- */

int PthreadJoinManager::DetachThread(pthread_t thread) 
{
    /* cannot detach a detached thread */
    if (!IsJoinable(thread))
    {
        std::cout << "[ERROR] Detaching a detached thread: "
                  << dec << thread << "!\n" << flush;
        return EINVAL;
    }
    /* only detach if no other threads are trying to join */
    else if (!IsJoined(thread)) 
    {
        joinables.erase(thread);
    }
    return 0;
}


/* --------------------------------------------------------------------------- */
/* JoinThreads:                                                                */
/* join the joiner thread with the joinee thread                               */
/* return true if joiner thread is blocked, false otherwise                    */
/* --------------------------------------------------------------------------- */

bool PthreadJoinManager::JoinThreads(pthread_t joiner, pthread_t joinee, void** retvalptr) 
{
    /* EDEADLK: thread cannot join itself */
    if (joiner == joinee) 
    {
        std::cout << "[ERROR] Thread " << dec << joiner
                  << " cannot join itself!\n" << flush;
        ASSERTX(0);
    }
    /* EINVAL/ESRCH: the joiner thread is nonexistent or has been detached */
    else if (!IsJoinable(joinee)) 
    {
        std::cout << "[ERROR] Thread " << dec << joinee
                  <<  " cannot be found/joined!\n" << flush;
        ASSERTX(0);
    }
    /* EINVAL: another thread cannot be already joining on the joinee thread */
    else if (IsJoined(joinee)) 
    {
        std::cout << "[ERROR] Thread " << dec << joinee
                  << " is already being joined by thread "
                  << GetJoiner(joinee) << "!\n" << flush;
        ASSERTX(0);
    }
    /* the joinee thread is done, just grab the return value */
    if (IsDone(joinee)) 
    {
        if (retvalptr != NULL) 
        {
            *retvalptr = GetRetval(joinee);
        }
        EraseJoinee(joinee);
        return false;
    }
    /* the joinee thread is not done yet, block the joiner thread */
    else 
    {
        SetJoiner(joinee, joiner);
        InsertJoiner(joiner, joinee, retvalptr);
        return true;
    }
}

/* --------------------------------------------------------------------------- */
/* GetAttr:                                                                    */
/* return the given thread's attribute fields relevant to joining              */
/* --------------------------------------------------------------------------- */

void PthreadJoinManager::GetAttr(pthread_t thread, pthread_attr_t* attr) 
{
    if (IsJoinable(thread)) 
    {
        PthreadAttr::pthread_attr_setdetachstate(attr, PTHREAD_CREATE_JOINABLE);
    }
    else 
    {
        PthreadAttr::pthread_attr_setdetachstate(attr, PTHREAD_CREATE_DETACHED);
    }
}

/* --------------------------------------------------------------------------- */
/* PthreadJoiner/Joinee Constructors and Destructors                           */
/* --------------------------------------------------------------------------- */

PthreadJoiner::PthreadJoiner(pthread_t _joinee, void** _retvalptr) :
    joinee(_joinee),
    retvalptr(_retvalptr)       {}

PthreadJoiner::~PthreadJoiner() {}

PthreadJoinee::PthreadJoinee() :
    done(false),
    joined(false)               {}

PthreadJoinee::~PthreadJoinee() {}

/* --------------------------------------------------------------------------- */
/* Functions for Manipulating STL Structure(s):                                */
/* --------------------------------------------------------------------------- */

pthreadjoinee_queue_t::iterator PthreadJoinManager::GetJoineeptr(pthread_t thread) 
{
    pthreadjoinee_queue_t::iterator joineeptr = joinables.find(thread);
    ASSERTX(joineeptr != joinables.end());
    return joineeptr;
}

void PthreadJoinManager::InsertJoinee(pthread_t thread) 
{
    ASSERTX(joinables.find(thread) == joinables.end());
    joinables[thread] = new PthreadJoinee();
}

void PthreadJoinManager::EraseJoinee(pthread_t thread) 
{
    pthreadjoinee_queue_t::iterator joineeptr = GetJoineeptr(thread);
    delete joineeptr->second;
    joinables.erase(joineeptr);
}

bool PthreadJoinManager::IsJoinable(pthread_t thread) 
{
    pthreadjoinee_queue_t::iterator joineeptr = joinables.find(thread);
    return (joineeptr != joinables.end());
}

bool PthreadJoinManager::IsDone(pthread_t thread) 
{
    pthreadjoinee_queue_t::iterator joineeptr = GetJoineeptr(thread);
    return ((joineeptr->second)->done);
}
    
bool PthreadJoinManager::IsJoined(pthread_t thread) 
{
    pthreadjoinee_queue_t::iterator joineeptr = GetJoineeptr(thread);
    return ((joineeptr->second)->joined);
}

pthread_t PthreadJoinManager::GetJoiner(pthread_t thread) 
{
    pthreadjoinee_queue_t::iterator joineeptr = GetJoineeptr(thread);
    ASSERTX((joineeptr->second)->joined);
    return ((joineeptr->second)->joiner);
}

void PthreadJoinManager::SetJoiner(pthread_t joinee, pthread_t joiner) 
{
    pthreadjoinee_queue_t::iterator joineeptr = GetJoineeptr(joinee);
    (joineeptr->second)->joined = true;
    (joineeptr->second)->joiner = joiner;
}

void* PthreadJoinManager::GetRetval(pthread_t thread) 
{
    pthreadjoinee_queue_t::iterator joineeptr = GetJoineeptr(thread);
    return ((joineeptr->second)->retval);
}

void PthreadJoinManager::SetRetval(pthread_t thread, void* retval) 
{
    pthreadjoinee_queue_t::iterator joineeptr = GetJoineeptr(thread);
    (joineeptr->second)->retval = retval;
    (joineeptr->second)->done = true;
}

pthreadjoiner_queue_t::iterator PthreadJoinManager::GetJoinerptr(pthread_t thread) 
{
    pthreadjoiner_queue_t::iterator joinerptr = joinings.find(thread);
    ASSERTX(joinerptr != joinings.end());
    return joinerptr;
}

void PthreadJoinManager::InsertJoiner(pthread_t joiner, pthread_t joinee, void** retvalptr) 
{
    ASSERTX(joinings.find(joiner) == joinings.end());
    joinings[joiner] = new PthreadJoiner(joinee, retvalptr);
}

void PthreadJoinManager::EraseJoiner(pthread_t thread) 
{
    pthreadjoiner_queue_t::iterator joinerptr = GetJoinerptr(thread);
    delete joinerptr->second;
    joinings.erase(joinerptr);
}

pthread_t PthreadJoinManager::GetJoinee(pthread_t thread) 
{
    pthreadjoiner_queue_t::iterator joinerptr = GetJoinerptr(thread);
    return ((joinerptr->second)->joinee);
}

void** PthreadJoinManager::GetRetvalptr(pthread_t thread) 
{
    pthreadjoiner_queue_t::iterator joinerptr = GetJoinerptr(thread);
    return ((joinerptr->second)->retvalptr);
}






    
    
        
