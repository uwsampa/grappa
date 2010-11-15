#include "PthreadAttr.h"

using namespace PinPthread;

/* --------------------------------------------------------------------------- */
/* PTHREAD_ATTR_DEFAULT:                                                       */
/* return a default pthread attribute object                                   */
/* --------------------------------------------------------------------------- */

pthread_attr_t PthreadAttr::PTHREAD_ATTR_DEFAULT() 
{
    pthread_attr_t attr;
    attr.__detachstate = PTHREAD_CREATE_JOINABLE;
    attr.__schedpolicy = SCHED_OTHER;
    attr.__inheritsched = PTHREAD_EXPLICIT_SCHED;
    attr.__scope = PTHREAD_SCOPE_SYSTEM;
    attr.__stackaddr = 0;
    attr.__stacksize = PTHREAD_STACK_MIN;
    return attr;
}

/* --------------------------------------------------------------------------- */
/* pthread_attr_destroy:                                                       */
/* destroy a thread attr object, which must not be reused until it is          */
/* reinitialized; but like the linuxthreads implementation, do nothing         */
/* --------------------------------------------------------------------------- */

int PthreadAttr::pthread_attr_destroy(pthread_attr_t* attr) 
{
    return 0;
}

/* --------------------------------------------------------------------------- */
/* pthread_attr_getdetachstate:                                                */
/* return the attribute's detach state                                         */
/* --------------------------------------------------------------------------- */

int PthreadAttr::pthread_attr_getdetachstate(pthread_attr_t* attr, int* detachstate) 
{
    ASSERTX(attr != NULL);
    ASSERTX(detachstate != NULL);
    *detachstate = attr->__detachstate;
    return 0;
}

/* --------------------------------------------------------------------------- */
/* pthread_attr_getstack:                                                      */
/* return the attribute's stack address and size                               */
/* --------------------------------------------------------------------------- */

int PthreadAttr::pthread_attr_getstack(pthread_attr_t* attr, void** stackaddr, size_t* stacksize) 
{
    pthread_attr_getstackaddr(attr, stackaddr);
    pthread_attr_getstacksize(attr, stacksize);
    return 0;
}

/* --------------------------------------------------------------------------- */
/* pthread_attr_getstackaddr:                                                  */
/* return the attribute's stack address                                        */
/* --------------------------------------------------------------------------- */

int PthreadAttr::pthread_attr_getstackaddr(pthread_attr_t* attr, void** stackaddr) 
{
    ASSERTX(attr != NULL);
    ASSERTX(stackaddr != NULL);
    *stackaddr = attr->__stackaddr;
    return 0;
}
    
/* --------------------------------------------------------------------------- */
/* pthread_attr_getstacksize:                                                  */
/* return the attribute's stack size                                           */
/* --------------------------------------------------------------------------- */

int PthreadAttr::pthread_attr_getstacksize(pthread_attr_t* attr, size_t* stacksize) 
{
    ASSERTX(attr != NULL);
    ASSERTX(stacksize != NULL);
    *stacksize = attr->__stacksize;
    return 0;
}

/* --------------------------------------------------------------------------- */
/* pthread_attr_init:                                                          */
/* initialize the given attribute with the default parameters                  */
/* --------------------------------------------------------------------------- */

int PthreadAttr::pthread_attr_init(pthread_attr_t* attr) 
{
    ASSERTX(attr != NULL);
    *attr = PTHREAD_ATTR_DEFAULT();
    return 0;
}

/* --------------------------------------------------------------------------- */
/* pthread_attr_setdetachstate:                                                */
/* set the attribute's detach state                                            */
/* --------------------------------------------------------------------------- */

int PthreadAttr::pthread_attr_setdetachstate(pthread_attr_t* attr, int detachstate) 
{
    ASSERTX(attr != NULL);
    attr->__detachstate = detachstate;
    if ((detachstate == PTHREAD_CREATE_JOINABLE) ||
        (detachstate == PTHREAD_CREATE_DETACHED)) 
    {
        return 0;
    }
    else 
    {
        std::cout << "[ERROR] Invalid detach state: " << dec << detachstate << "\n" << flush;
        return EINVAL;
    }
}

/* --------------------------------------------------------------------------- */
/* pthread_attr_setstack:                                                      */
/* set the attribute's stack address and size                                  */
/* --------------------------------------------------------------------------- */

int PthreadAttr::pthread_attr_setstack(pthread_attr_t* attr, void* stackaddr, size_t stacksize) 
{
    int error = 0;
    error += pthread_attr_setstackaddr(attr, stackaddr);
    error += pthread_attr_setstacksize(attr, stacksize);
    return error;
}

/* --------------------------------------------------------------------------- */
/* pthread_attr_setstackaddr:                                                  */
/* set the attribute's stack address to the given address                      */
/* --------------------------------------------------------------------------- */

int PthreadAttr::pthread_attr_setstackaddr(pthread_attr_t* attr, void* stackaddr) 
{
    ASSERTX(attr != NULL);
    attr->__stackaddr = stackaddr;
    if (((ADDRINT)stackaddr & 0x7) != 0) 
    {
        std::cout << "[ERROR] Stack address (" << hex << (ADDRINT)stackaddr
                  << ") is not aligned!\n" << flush;
        return EINVAL;
    }
    else 
    {
        return 0;
    }
}

/* --------------------------------------------------------------------------- */
/* pthread_attr_setstacksize:                                                  */
/* set the attribute's stack size to the given size                            */
/* --------------------------------------------------------------------------- */

int PthreadAttr::pthread_attr_setstacksize(pthread_attr_t* attr, size_t stacksize) 
{
    ASSERTX(attr != NULL);
    attr->__stacksize = stacksize;
    if (stacksize < PTHREAD_STACK_MIN) 
    {
        std::cout << "[ERROR] Stack size (" << hex << stacksize << ") is too small - "
                  << "PTHREAD_STACK_MIN = " << PTHREAD_STACK_MIN << "\n" << flush;
        return EINVAL;
    }
    else
    {
        return 0;
    }
}

