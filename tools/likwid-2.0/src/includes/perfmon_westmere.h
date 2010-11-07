/*
 * ===========================================================================
 *
 *      Filename:  perfmon_westmere.h
 *
 *      Description:  Header File of perfmon module for Westmere.
 *                    Configures and reads out performance counters
 *                    on x86 based architectures. Supports multi threading.
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
#include <stdlib.h>
#include <stdio.h>

#include <cpuid.h>
#include <tree.h>
#include <bstrlib.h>
#include <types.h>
#include <registers.h>
#include <perfmon_westmere_events.h>
#include <perfmon_westmere_groups.h>

static int perfmon_numGroupsWestmere = NUM_GROUPS_WESTMERE;
static int perfmon_numArchEventsWestmere = NUM_ARCH_EVENTS_WESTMERE;

