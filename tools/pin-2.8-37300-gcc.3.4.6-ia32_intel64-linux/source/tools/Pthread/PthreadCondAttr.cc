#include "PthreadCondAttr.h"

using namespace PinPthread;

/* NOTE: like the LinuxThreads implementation, condition attributes are just   */
/* dummy objects in this pthread library implementation                        */

int PthreadCondAttr::pthread_condattr_destroy(pthread_condattr_t* attr) { return 0; }

int PthreadCondAttr::pthread_condattr_init(pthread_condattr_t* attr)    { return 0; }

    
