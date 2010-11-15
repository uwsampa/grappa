#include "PthreadOnce.h"

using namespace PinPthread;

/* --------------------------------------------------------------------------- */
/* pthread_once:                                                               */
/* the first time pthread_once is called with a given once_control argument,   */
/* it calls init_routine with no argument; subsequent calls to pthread_once    */
/* with the same once_control argument do nothing                              */
/* --------------------------------------------------------------------------- */

void PthreadOnce::pthread_once(pthread_once_t* once_control,
                               ADDRINT init_routine, CONTEXT* ctxt)
{
    ASSERTX(once_control != NULL);
    if (*once_control == PTHREAD_ONCE_INIT) 
    {
#if VERBOSE
        std::cout << "Once_control " << hex << once_control
                  << " - calling init_routine " << init_routine << "\n" << flush;
#endif
        *once_control = PTHREAD_ONCE_DONE;
        PIN_SetContextReg(ctxt, REG_INST_PTR, init_routine);
        PIN_ExecuteAt(ctxt);
    }
#if VERBOSE
    else 
    {
        std::cout << "Once_control " << hex << once_control
                  << " has already been init\n" << flush;
    }
#endif
}

