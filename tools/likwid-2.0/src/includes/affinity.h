/*
 * ===========================================================================
 *
 *      Filename:  affinity.h
 *
 *      Description:  Header File affinity Module
 *
 *      Version:  2.0
 *      Created:  12.10.2010
 *
 *      Author:  Jan Treibig (jt), jan.treibig@gmail.com
 *      Company:  RRZE Erlangen
 *      Project:  likwid
 *      Copyright:  Copyright (c) 2010, Jan Treibig
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License, v2, as
 *      published by the Free Software Foundation
 *     
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *     
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * ===========================================================================
 */

#ifndef AFFINITY_H
#define AFFINITY_H

#include <types.h>

extern void affinity_init();
extern void affinity_finalize();
extern int  affinity_processGetProcessorId();
extern int  affinity_threadGetProcessorId();
extern void  affinity_pinProcess(int processorId);
extern void  affinity_pinThread(int processorId);
extern const AffinityDomain* affinity_getDomain(bstring domain);
extern void affinity_printDomains();

#endif /*AFFINITY_H*/

