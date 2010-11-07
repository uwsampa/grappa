/*
 * ===========================================================================
 *
 *      Filename:  likwid.h
 *
 *      Description:  Header File of likwid marker API
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

#ifndef LIKWID_H
#define LIKWID_H

extern void likwid_markerInit(int numberOfThreads, int numberOfRegions);
extern void likwid_markerClose(void);
extern void likwid_markerStartRegion(int thread_id, int core_id);
extern void likwid_markerStopRegion(int thread_id, int core_id, int regionTag);
extern int  likwid_markerRegisterRegion(char* regionTag);
extern int  likwid_markerGetRegionId(char* regionTag);

extern int  likwid_processGetProcessorId();
extern int  likwid_threadGetProcessorId();
extern int  likwid_pinProcess(int processorId);
extern int  likwid_pinThread(int processorId);

#endif /*LIKWID_H*/
