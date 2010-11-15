#include "PthreadCleanup.h"

using namespace PinPthread;

/* --------------------------------------------------------------------------- */
/* PushHandler:                                                                */
/* install the routine function with argument arg as a cleanup handler         */
/* --------------------------------------------------------------------------- */

void PthreadCleanupManager::PushHandler(pthread_t thread, ADDRINT func, ADDRINT arg) 
{
    PthreadCleanupHandlers* handlerlifo = GetHandlerLIFO(thread);
    handlerlifo->PushHandler(func, arg);
}

/* --------------------------------------------------------------------------- */
/* PopHandler:                                                                 */
/* remove the most recently installed cleanup handler                          */
/* also execute the handler if execute is not 0                                */
/* --------------------------------------------------------------------------- */

void PthreadCleanupManager::PopHandler(pthread_t thread, int execute, CONTEXT* ctxt) 
{
    if (handlers.find(thread) == handlers.end()) 
    {
        return;
    }
    PthreadCleanupHandlers* handlerlifo = GetHandlerLIFO(thread);
    if (handlerlifo->HasMoreHandlers()) 
    {
        pthread_handler_t handler = handlerlifo->PopHandler();
        if (execute) 
        {
#if VERBOSE
            std::cout << "Executing Cleanup Handler (rtn=" << hex << handler.first
                      << ", arg=" << handler.second << ")\n" << flush;
#endif            
            ADDRINT* sp = (ADDRINT*)PIN_GetContextReg(ctxt, REG_STACK_PTR);
            *(sp--) = handler.second;
            *(sp--) = handler.first;
            PIN_SetContextReg(ctxt, REG_STACK_PTR, (ADDRINT)sp);
            PIN_SetContextReg(ctxt, REG_INST_PTR, (ADDRINT)StartThreadFunc);
            PIN_ExecuteAt(ctxt);
        }
    }
}

/* --------------------------------------------------------------------------- */
/* PthreadCleanupHandlers Functions:                                           */
/* --------------------------------------------------------------------------- */

void PthreadCleanupHandlers::PushHandler(ADDRINT func, ADDRINT arg) 
{
    pthread_handler_t handler;
    handler.first = func;
    handler.second = arg;
    handlerlifo.push_back(handler);
}

pthread_handler_t PthreadCleanupHandlers::PopHandler() 
{
    ASSERTX(!handlerlifo.empty());
    pthread_handler_t handler = handlerlifo.back();
    handlerlifo.pop_back();
    return handler;
}

bool PthreadCleanupHandlers::HasMoreHandlers() 
{
    return (!handlerlifo.empty());
}

/* --------------------------------------------------------------------------- */
/* PthreadCleanupManager Helper Function(s):                                   */
/* --------------------------------------------------------------------------- */

PthreadCleanupHandlers* PthreadCleanupManager::GetHandlerLIFO(pthread_t thread) 
{
    pthreadhandler_queue_t::iterator threadptr = handlers.find(thread);
    PthreadCleanupHandlers* handlersptr;
    if (threadptr == handlers.end()) 
    {
        handlersptr = new PthreadCleanupHandlers();
        handlers[thread] = handlersptr;
    }
    else
    {
        handlersptr = threadptr->second;
    }
    return handlersptr;
}

