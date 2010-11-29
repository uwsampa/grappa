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


/******************************************************************************
 *                                                                            *
 * SpinBarrier                                                                *
 *                                                                            *
 * Author:  Douglas M. Pase                                                   *
 *                                                                            *
 * Date:    September 21, 2000                                                *
 *          Translated to C++, June 19, 2005                                  *
 *          Rewritten August 13,2005                                          *
 *                                                                            *
 *  void barrier()                                                            *
 *                                                                            *
 ******************************************************************************/

#if !defined( SpinBarrier_h )
#define SpinBarrier_h

#include <pthread.h>

class SpinBarrier {
public:
    SpinBarrier(int participants);
    ~SpinBarrier();

    void barrier();

private:
    int limit;				// number of barrier participants
    pthread_barrier_t barrier_obj;
};

#endif
