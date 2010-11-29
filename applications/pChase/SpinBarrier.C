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
 *                                                                            *
 *  void barrier()                                                            *
 *                                                                            *
 ******************************************************************************/
#include <stdio.h>
#include <pthread.h>

#include "SpinBarrier.h"

				// create a new barrier
SpinBarrier::SpinBarrier(int participants)
: limit( participants )
{
    pthread_barrier_init( &barrier_obj, NULL, this->limit );
}

				// destroy an old barrier
SpinBarrier::~SpinBarrier()
{
}

				// enter the barrier and wait.  everyone leaves 
				// when the last participant enters the barrier.
void
SpinBarrier::barrier()
{
    pthread_barrier_wait( &this->barrier_obj );
}
