#include "PthreadKey.h"

using namespace PinPthread;

/* =========================================================================== */
/* PthreadTLSManager:                                                          */
/* =========================================================================== */

/* --------------------------------------------------------------------------- */
/* Constructor and Destructor:                                                 */
/* --------------------------------------------------------------------------- */

PthreadTLSManager::PthreadTLSManager() : newkey(1) {}

PthreadTLSManager::~PthreadTLSManager()            {}

/* --------------------------------------------------------------------------- */
/* AddThread:                                                                  */
/* register a new thread                                                       */
/* --------------------------------------------------------------------------- */

void PthreadTLSManager::AddThread(pthread_t thread) 
{
    ASSERTX(tls.find(thread) == tls.end());
    PthreadTLS* thread_tls = new PthreadTLS();
    pthread_keydestr_t::iterator itr;
    for (itr = destr.begin(); itr != destr.end(); itr++) 
    {
        thread_tls->AddKey(itr->first);
    }
    tls[thread] = thread_tls;
}

/* --------------------------------------------------------------------------- */
/* KillThread:                                                                 */
/* deallocate a thread                                                         */
/* run the destructor associated with each key, in arbitrary order             */
/* --------------------------------------------------------------------------- */

void PthreadTLSManager::KillThread(pthread_t thread) 
{
    PthreadTLS* thread_tls = GetTLS(thread);
    pthread_keydestr_t::iterator itr;
    if (thread != 0) 
    {
        for (itr = destr.begin(); itr != destr.end(); itr++) 
        {
            void(*func)(void*) = itr->second;
            if (func != NULL) 
            {
                func(thread_tls->GetData(itr->first));
            }
        }
    }
    delete thread_tls;
    tls.erase(thread);
}

/* --------------------------------------------------------------------------- */
/* AddKey:                                                                     */
/* implement pthread_key_create                                                */
/* --------------------------------------------------------------------------- */

int PthreadTLSManager::AddKey(pthread_key_t* key, void(*func)(void*)) 
{
    ASSERTX(key != NULL);
    *key = newkey;
    newkey++;
    if (tls.size() >= PTHREAD_KEYS_MAX) 
    {
        std::cout << "[ERROR] Not Creating Key " << dec << *key << " Because " 
                  << PTHREAD_KEYS_MAX << " Keys Are Already Allocated\n" << flush;
        return EAGAIN;
    }
    else 
    {
        pthreadtls_queue_t::iterator itr;
        for (itr = tls.begin(); itr != tls.end(); itr++) 
        {
            (itr->second)->AddKey(*key);
        }
        ASSERTX(destr.find(*key) == destr.end());
        destr[*key] = func;
        return 0;
    }
}

/* --------------------------------------------------------------------------- */
/* RemoveKey:                                                                  */
/* implement pthread_key_delete                                                */
/* --------------------------------------------------------------------------- */

int PthreadTLSManager::RemoveKey(pthread_key_t key) 
{
    bool success = true;
    pthreadtls_queue_t::iterator itr;
    for (itr = tls.begin(); itr != tls.end(); itr++) 
    {
        success &= (itr->second)->RemoveKey(key);
    }
    success &= (destr.find(key) != destr.end());
    destr.erase(key);
    if (success)
    {
        return 0;
    }
    else 
    {
        std::cout << "[ERROR] pthread_key_delete: Can't Find Key " << dec << key << "\n" << flush;
        return EINVAL;
    }
}

/* --------------------------------------------------------------------------- */
/* SetData:                                                                    */
/* implement pthread_setspecific                                               */
/* --------------------------------------------------------------------------- */

int PthreadTLSManager::SetData(pthread_t thread, pthread_key_t key, void* data) 
{
    PthreadTLS* thread_tls = GetTLS(thread);
    int error = thread_tls->SetData(key, data);
    if (error == EINVAL) 
    {
        std::cout << "[ERROR] pthread_setspecific: Can't Find Key " << dec << key
                  << " For Thread " << dec << thread << "\n" << flush;
    }
#if VERYVERBOSE
    else 
    {
        std::cout << "Key " << hex << key << " For Thread " << dec << thread
                  << " Set to Dataptr " << hex << data << "\n" << flush;
    }
#endif
    return error;
}

/* --------------------------------------------------------------------------- */
/* GetData:                                                                    */
/* implement pthread_getspecific                                               */
/* --------------------------------------------------------------------------- */

void* PthreadTLSManager::GetData(pthread_t thread, pthread_key_t key)
{
    PthreadTLS* thread_tls = GetTLS(thread);
    void* data = thread_tls->GetData(key);
    if ((data == NULL) && (key != 0)) 
    {
        std::cout << "[ERROR] pthread_getspecific: Can't Find Key " << dec << key
                  << " For Thread " << dec << thread << "\n" << flush;
    }
#if VERYVERBOSE
    else 
    {
        std::cout << "Key " << hex << key << " For Thread " << dec << thread
                  << " Matches Dataptr " << hex << data << "\n" << flush;
    }
#endif
    return data;
}

/* --------------------------------------------------------------------------- */
/* GetTLS:                                                                     */
/* return the TLS object associated with this thread                           */
/* --------------------------------------------------------------------------- */

PthreadTLS* PthreadTLSManager::GetTLS(pthread_t thread) 
{
    pthreadtls_queue_t::iterator threadptr = tls.find(thread);
    ASSERTX(threadptr != tls.end());
    return threadptr->second;
}

/* =========================================================================== */
/* PthreadTLS:                                                                 */
/* =========================================================================== */

/* --------------------------------------------------------------------------- */
/* AddKey:                                                                     */
/* allocate a new TLS key, with initial data value of NULL                     */
/* --------------------------------------------------------------------------- */

void PthreadTLS::AddKey(pthread_key_t key)
{
    if (tls.find(key) == tls.end()) 
    {
        tls[key] = NULL;
    }
    else 
    {
        ASSERTX(key == 0);
    }
}

/* --------------------------------------------------------------------------- */
/* RemoveKey:                                                                  */
/* deallocate a TLS key                                                        */
/* --------------------------------------------------------------------------- */

bool PthreadTLS::RemoveKey(pthread_key_t key) 
{
    if (tls.find(key) == tls.end())
    {
        return false;
    }
    else 
    {
        tls.erase(key);
        return true;
    }

}

/* --------------------------------------------------------------------------- */
/* SetData:                                                                    */
/* associate the given key with the given data                                 */
/* --------------------------------------------------------------------------- */

int PthreadTLS::SetData(pthread_key_t key, void* data) 
{
    if ((tls.find(key) == tls.end()) && (key != 0))
    {
        return EINVAL;
    }
    else 
    {
        tls[key] = data;
        return 0;
    }
}

/* --------------------------------------------------------------------------- */
/* GetData:                                                                    */
/* return the value associated with the given key                              */
/* if the key is not valid, return NULL                                        */
/* --------------------------------------------------------------------------- */

void* PthreadTLS::GetData(pthread_key_t key) 
{
    if (tls.find(key) == tls.end()) 
    {
        return NULL;
    }
    else 
    {
        return tls[key];
    }
}
    
