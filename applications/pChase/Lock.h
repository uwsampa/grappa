/*******************************************************************************
 * Copyright (c) 2006 International Business Machines Corporation.             *
 * All rights reserved. This program and the accompanying materials            *
 * are made available under the terms of the Common Public License v1.0        *
 * which accompanies this distribution, and is available at                    *
 * http://www.opensource.org/licenses/cpl1.0.php                               *
 *                                                                             *
 * Contributors:                                                               *
 *    Douglas M. Pase - initial API and implementation                         *
 *******************************************************************************/


#if !defined(Lock_h)
#define Lock_h

#include <pthread.h>

class Lock {
public:
    Lock();
    ~Lock();
    void lock();
    int test();
    void unlock();

private:
    pthread_mutex_t mutex;
};

#endif
