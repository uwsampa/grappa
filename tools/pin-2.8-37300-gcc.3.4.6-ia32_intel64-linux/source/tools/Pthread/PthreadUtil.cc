#include "PthreadUtil.h"

namespace PinPthread
{

/* --------------------------------------------------------------------------- */
/* StartThreadFunc:                                                            */
/* wrapper function for the thread func to catch the end of the thread         */
/* --------------------------------------------------------------------------- */

void StartThreadFunc(void*(*func)(void*), void* arg)
{
    void* retval = func(arg);
    pthread_exit(retval);
}

} // namespace PinPthread
