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


#if !defined(Timer_h)
#define Timer_h

#include "Types.h"

class Timer {
public:
    static double seconds();
    static double resolution();
    static int64  ticks();
    static void   calibrate();
    static void   calibrate(int n);
private:
};

#endif
