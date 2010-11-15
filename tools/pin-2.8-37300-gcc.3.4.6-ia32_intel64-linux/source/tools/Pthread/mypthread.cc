typedef unsigned long int pthread_t;
typedef void*             pthread_attr_t;
typedef void*             pthread_cond_t;
typedef void*             pthread_condattr_t;
typedef void*             pthread_mutex_t;
typedef void*             pthread_mutexattr_t;
typedef unsigned long int pthread_key_t;
typedef int               pthread_once_t;
typedef unsigned long int size_t;
typedef void*             sigset_t;

extern "C" 
{

int       pthread_atfork(void(*)(void), void(*)(void), void(*)(void))                      { return 0; }
int       pthread_attr_destroy(pthread_attr_t*)                                            { return 0; }
int       pthread_attr_getdetachstate(pthread_attr_t*, int*)                               { return 0; }
int       pthread_attr_getstack(pthread_attr_t*, void*, size_t)                            { return 0; }
int       pthread_attr_getstackaddr(pthread_attr_t*, void**)                               { return 0; }
int       pthread_attr_getstacksize(pthread_attr_t*, size_t*)                              { return 0; }   
int       pthread_attr_init(pthread_attr_t*)                                               { return 0; }
int       pthread_attr_setdetachstate(pthread_attr_t*, int)                                { return 0; }
int       pthread_attr_setstack(pthread_attr_t*, void*, size_t)                            { return 0; }
int       pthread_attr_setstackaddr(pthread_attr_t*, void*)                                { return 0; }
int       pthread_attr_setstacksize(pthread_attr_t*, size_t)                               { return 0; }
int       pthread_cancel(pthread_t)                                                        { return 0; }
void      pthread_cleanup_pop(int)                                                         {           }
void      pthread_cleanup_push(void(*)(void*), void*)                                      {           }
int       pthread_condattr_destroy(pthread_condattr_t*)                                    { return 0; }
int       pthread_condattr_init(pthread_condattr_t*)                                       { return 0; }
int       pthread_cond_broadcast(pthread_cond_t*)                                          { return 0; }
int       pthread_cond_destroy(pthread_cond_t*)                                            { return 0; }
int       pthread_cond_init(pthread_cond_t*, pthread_condattr_t*)                          { return 0; }
int       pthread_cond_signal(pthread_cond_t*)                                             { return 0; }
int       pthread_cond_timedwait(pthread_cond_t*, pthread_mutex_t*, const struct timespec*){ return 0; }  // may not return
int       pthread_cond_wait(pthread_cond_t*, pthread_mutex_t*)                             { return 0; }  // may not return
int       pthread_create(pthread_t*, const pthread_attr_t*, void*(*)(void*), void*)        { return 0; }  // may not return
int       pthread_detach(pthread_t)                                                        { return 0; }
int       pthread_equal(pthread_t, pthread_t)                                              { return 0; }
void      pthread_exit(void*)                                                              {           }  // does not return
int       pthread_getattr_np(pthread_t, pthread_attr_t*)                                   { return 0; }
void*     pthread_getspecific(pthread_key_t)                                               { return 0; }
int       pthread_join(pthread_t, void**)                                                  { return 0; }  // may not return
int       pthread_key_create(pthread_key_t*, void(*)(void*))                               { return 0; }
int       pthread_key_delete(pthread_key_t)                                                { return 0; }
int       pthread_kill(pthread_t, int)                                                     { return 0; }
int       pthread_mutexattr_destroy(pthread_mutexattr_t*)                                  { return 0; }
int       pthread_mutexattr_getkind_np(pthread_mutexattr_t*, int*)                         { return 0; }
int       pthread_mutexattr_gettype(pthread_mutexattr_t*, int*)                            { return 0; }
int       pthread_mutexattr_init(pthread_mutexattr_t*)                                     { return 0; }
int       pthread_mutexattr_setkind_np(pthread_mutexattr_t*, int)                          { return 0; }
int       pthread_mutexattr_settype(pthread_mutexattr_t*, int)                             { return 0; }
int       pthread_mutex_destroy(pthread_mutex_t*)                                          { return 0; }
int       pthread_mutex_init(pthread_mutex_t*, const pthread_mutexattr_t*)                 { return 0; }
int       pthread_mutex_lock(pthread_mutex_t*)                                             { return 0; }  // may not return
int       pthread_mutex_trylock(pthread_mutex_t*)                                          { return 0; }
int       pthread_mutex_unlock(pthread_mutex_t*)                                           { return 0; }
int       pthread_once(pthread_once_t*, void(*)(void))                                     { return 0; }  // may not return
pthread_t pthread_self()                                                                   { return 0; } 
int       pthread_setcancelstate(int, int*)                                                { return 0; }
int       pthread_setcanceltype(int, int*)                                                 { return 0; } 
int       pthread_setspecific(pthread_key_t, void*)                                        { return 0; }
void      pthread_testcancel()                                                             {           }  // may not return

void*     libc_tsd_get(int)                                                                { return 0; }
void      libc_tsd_set(int, void*)                                                         {           }
void*     (*__libc_internal_tsd_get)(int) = libc_tsd_get;
void      (*__libc_internal_tsd_set)(int, void*) = libc_tsd_set;

/* -------------------------------------------------------------------------------------------------- */

int       pthread_sigmask(int, const sigset_t*, sigset_t*)                                 { return 0; }
int       sigwait(const sigset_t*, int*)                                                   { return 0; }

int       pthread_attr_setschedparam(pthread_attr_t*, struct sched_param*)                 { return 0; }
int       pthread_attr_getschedparam(pthread_attr_t*, struct sched_param*)                 { return 0; }
int       pthread_attr_setschedpolicy(pthread_attr_t*, int)                                { return 0; }
int       pthread_attr_getschedpolicy(pthread_attr_t*, int*)                               { return 0; }
int       pthread_attr_setinheritsched(pthread_attr_t*, int)                               { return 0; }
int       pthread_attr_getinheritsched(pthread_attr_t*, int*)                              { return 0; }
int       pthread_attr_setscope(pthread_attr_t*, int)                                      { return 0; }
int       pthread_attr_getscope(pthread_attr_t*, int*)                                     { return 0; }
int       pthread_attr_setguardsize(pthread_attr_t*, size_t)                               { return 0; }
int       pthread_attr_getguardsize(pthread_attr_t*, size_t*)                              { return 0; }

int       pthread_setschedparam(pthread_t, int, struct sched_param*)                       { return 0; }
int       pthread_getschedparam(pthread_t, int*, struct sched_param*)                      { return 0; }

int       pthread_getconcurrency()                                                         { return 0; }
int       pthread_setconcurrency(int)                                                      { return 0; }

int       pthread_yield()                                                                  { return 0; }

int       pthread_mutex_timedlock(pthread_mutex_t*, struct timespec*)                      { return 0; }

int       pthread_mutexattr_getpshared(pthread_mutexattr_t*, int*)                         { return 0; }
int       pthread_mutexattr_setpshared(pthread_mutexattr_t*, int)                          { return 0; }
int       pthread_condattr_getpshared(pthread_condattr_t*, int*)                           { return 0; }
int       pthread_condattr_setpshared(pthread_condattr_t*, int)                            { return 0; }

void      pthread_cleanup_push_defer_np(void(*)(void*), void*)                             {           }
void      pthread_cleanup_pop_restore_np(int)                                              {           }

void      pthread_kill_other_threads_np()                                                  {           }

}
