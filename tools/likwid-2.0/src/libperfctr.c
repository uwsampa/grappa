/*
 * ===========================================================================
 *
 *      Filename:  libperfctr.c
 *
 *      Description:  Marker API interface of module perfmon
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


/* #####   HEADER FILE INCLUDES   ######################################### */

#include <stdlib.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sched.h>
#include <pthread.h>

#include <error.h>
#include <types.h>
#include <bstrlib.h>
#include <cpuid.h>
#include <tree.h>
#include <msr.h>
#include <timer.h>
#include <registers.h>
#include <likwid.h>

static LikwidResults*  likwid_results;
static CyclesData*  likwid_time;
static int likwid_numberOfRegions = 0;
static int likwid_numberOfThreads = 0;

static volatile int nehalem_socket_lock[MAX_NUM_SOCKETS];
static int nehalem_processor_lookup[MAX_NUM_THREADS];

/* #####   MACROS  -  LOCAL TO THIS SOURCE FILE   ######################### */

#define gettid() syscall(SYS_gettid)

/* #####   FUNCTION DEFINITIONS  -  LOCAL TO THIS SOURCE FILE   ########### */

static int
getProcessorID(cpu_set_t* cpu_set)
{
    int processorId;

    for (processorId=0;processorId<24;processorId++){
        if (CPU_ISSET(processorId,cpu_set))
        {  
            break;
        }
    }
    return processorId;
}

/* #####   FUNCTION DEFINITIONS  -  EXPORTED FUNCTIONS   ################## */
void
likwid_markerInit(int numberOfThreads, int numberOfRegions)
{
    int i;
    int j;
    int k;
    TreeNode* socketNode;
    TreeNode* coreNode;
    TreeNode* threadNode;
    int socketId;

    cpuid_init();
    timer_init();

    switch ( cpuid_info.model ) 
    {
        case NEHALEM_WESTMERE_M:
        case NEHALEM_WESTMERE:
        case NEHALEM_BLOOMFIELD:
        case NEHALEM_LYNNFIELD:
            for(int i=0; i<MAX_NUM_SOCKETS; i++) nehalem_socket_lock[i] = 0;

            socketNode = tree_getChildNode(cpuid_topology.topologyTree);
            while (socketNode != NULL)
            {
                socketId = socketNode->id;
                coreNode = tree_getChildNode(socketNode);

                while (coreNode != NULL)
                {
                    threadNode = tree_getChildNode(coreNode);

                    while (threadNode != NULL)
                    {
                        nehalem_processor_lookup[threadNode->id] = socketId;
                        threadNode = tree_getNextNode(threadNode);
                    }
                    coreNode = tree_getNextNode(coreNode);
                }
                socketNode = tree_getNextNode(socketNode);
            }
            break;
    }

              
	likwid_numberOfThreads = numberOfThreads;
	likwid_numberOfRegions = numberOfRegions;

	// TODO: Pad data structures to prevent false shared cache lines between threads
    likwid_time = (CyclesData*) malloc(numberOfThreads * sizeof(CyclesData));
    likwid_results = (LikwidResults*) malloc(numberOfRegions * sizeof(LikwidResults));

    for (i=0;i<numberOfRegions; i++)
    {
        likwid_results[i].time = (double*) malloc(numberOfThreads * sizeof(double));
        likwid_results[i].counters = (double**) malloc(numberOfThreads * sizeof(double*));

        for (j=0;j<numberOfThreads; j++)
        {
            likwid_results[i].time[j] = 0.0;
            likwid_results[i].counters[j] = (double*) malloc(NUM_PMC * sizeof(double));
            for (k=0;k<NUM_PMC; k++)
            {
                likwid_results[i].counters[j][k] = 0.0;
            }
        }
    }
}

/* File format 
 * 1 numberOfThreads numberOfRegions
 * 2 regionID:regionTag0
 * 3 regionID:regionTag1
 * 4 regionID threadID countersvalues(space sparated)
 * 5 regionID threadID countersvalues
 */
void likwid_markerClose()
{
	int i,j,k;
    FILE *file = NULL;

    file = fopen(getenv("LIKWID_FILEPATH"),"w");
    if (file != NULL)
    {
        fprintf(file,"%d %d\n",likwid_numberOfThreads,likwid_numberOfRegions);

        for (i=0; i<likwid_numberOfRegions; i++)
        {
            fprintf(file,"%d:%s\n",i,bdata(likwid_results[i].tag));
        }

        for (i=0; i<likwid_numberOfRegions; i++)
        {
            for (j=0; j<likwid_numberOfThreads; j++)
            {
                fprintf(file,"%d ",i);
                fprintf(file,"%d ",j);
                fprintf(file,"%e ",likwid_results[i].time[j]);

                for (k=0; k<NUM_PMC; k++)
                {
                    fprintf(file,"%e ",likwid_results[i].counters[j][k]);
                }
                fprintf(file,"\n");
            }
        }
        fclose(file);
    }

    for (i=0;i<likwid_numberOfRegions; i++)
    {
        for (j=0;j<likwid_numberOfThreads; j++)
        {
            free(likwid_results[i].counters[j]);
        }
        free(likwid_results[i].time);
        free(likwid_results[i].counters);
    }

    free(likwid_results);
    free(likwid_time);
}

void
likwid_markerStartRegion(int thread_id, int cpu_id)
{
    uint64_t flags = 0x0ULL;

    switch ( cpuid_info.family ) 
    {
        case P6_FAMILY:

            switch ( cpuid_info.model ) 
            {
                case PENTIUM_M_BANIAS:
                    break;

                case PENTIUM_M_DOTHAN:
                    break;

                case CORE_DUO:
                    break;

                case XEON_MP:

                case CORE2_65:

                case CORE2_45:

                    msr_write(cpu_id, MSR_PERF_GLOBAL_CTRL, 0x0ULL);
                    msr_write(cpu_id, MSR_PERF_FIXED_CTR0, 0x0ULL);
                    msr_write(cpu_id, MSR_PERF_FIXED_CTR1, 0x0ULL);
                    msr_write(cpu_id, MSR_PMC0 , 0x0ULL);
                    msr_write(cpu_id, MSR_PMC1 , 0x0ULL);
                    msr_write(cpu_id, MSR_PERF_GLOBAL_CTRL, 0x300000003ULL);
                    msr_write(cpu_id, MSR_PERF_GLOBAL_OVF_CTRL, 0x300000003ULL);
                    break;

                case NEHALEM_WESTMERE_M:

                case NEHALEM_WESTMERE:

                case NEHALEM_BLOOMFIELD:

                case NEHALEM_LYNNFIELD:

                    msr_write(cpu_id, MSR_PERF_GLOBAL_CTRL, 0x0ULL);
                    msr_write(cpu_id, MSR_PERF_FIXED_CTR0, 0x0ULL);
                    msr_write(cpu_id, MSR_PERF_FIXED_CTR1, 0x0ULL);
                    msr_write(cpu_id, MSR_PERF_FIXED_CTR2, 0x0ULL);
                    msr_write(cpu_id, MSR_PMC0 , 0x0ULL);
                    msr_write(cpu_id, MSR_PMC1 , 0x0ULL);
                    msr_write(cpu_id, MSR_PMC2 , 0x0ULL);
                    msr_write(cpu_id, MSR_PMC3 , 0x0ULL);
                    msr_write(cpu_id, MSR_PERF_GLOBAL_CTRL, 0x70000000FULL);
                    msr_write(cpu_id, MSR_PERF_GLOBAL_OVF_CTRL, 0x30000000FULL);

                    if (!nehalem_socket_lock[nehalem_processor_lookup[cpu_id]])
                    {
                        nehalem_socket_lock[nehalem_processor_lookup[cpu_id]] = 1;
                        msr_write(cpu_id, MSR_UNCORE_PMC0, 0x0ULL);
                        msr_write(cpu_id, MSR_UNCORE_PMC1, 0x0ULL);
                        msr_write(cpu_id, MSR_UNCORE_PMC2, 0x0ULL);
                        msr_write(cpu_id, MSR_UNCORE_PMC3, 0x0ULL);
                        msr_write(cpu_id, MSR_UNCORE_PMC4, 0x0ULL);
                        msr_write(cpu_id, MSR_UNCORE_PMC5, 0x0ULL);
                        msr_write(cpu_id, MSR_UNCORE_PMC6, 0x0ULL);
                        msr_write(cpu_id, MSR_UNCORE_PMC7, 0x0ULL);
                        msr_write(cpu_id, MSR_UNCORE_PERF_GLOBAL_CTRL, 0x1000000FFULL);
                    }

                    break;

                case NEHALEM_EX:

                    msr_write(cpu_id, MSR_PERF_GLOBAL_CTRL, 0x0ULL);
                    msr_write(cpu_id, MSR_PERF_FIXED_CTR0, 0x0ULL);
                    msr_write(cpu_id, MSR_PERF_FIXED_CTR1, 0x0ULL);
                    msr_write(cpu_id, MSR_PERF_FIXED_CTR2, 0x0ULL);
                    msr_write(cpu_id, MSR_PMC0 , 0x0ULL);
                    msr_write(cpu_id, MSR_PMC1 , 0x0ULL);
                    msr_write(cpu_id, MSR_PMC2 , 0x0ULL);
                    msr_write(cpu_id, MSR_PMC3 , 0x0ULL);
                    msr_write(cpu_id, MSR_PERF_GLOBAL_CTRL, 0x70000000FULL);
                    msr_write(cpu_id, MSR_PERF_GLOBAL_OVF_CTRL, 0x30000000FULL);

                    break;

                default:
                    ERROR_MSG(Unsupported Processor);
                    break;
            }
            break;

        case K8_FAMILY:


        case K10_FAMILY:
            msr_write(cpu_id, MSR_AMD_PMC0 , 0x0ULL);
            msr_write(cpu_id, MSR_AMD_PMC1 , 0x0ULL);
            msr_write(cpu_id, MSR_AMD_PMC2 , 0x0ULL);
            msr_write(cpu_id, MSR_AMD_PMC3 , 0x0ULL);

            flags = msr_read(cpu_id, MSR_AMD_PERFEVTSEL0);
            flags |= (1<<22);  /* enable flag */
            msr_write(cpu_id, MSR_AMD_PERFEVTSEL0, flags);
            flags = msr_read(cpu_id, MSR_AMD_PERFEVTSEL1);
            flags |= (1<<22);  /* enable flag */
            msr_write(cpu_id, MSR_AMD_PERFEVTSEL1, flags);
            flags = msr_read(cpu_id, MSR_AMD_PERFEVTSEL2);
            flags |= (1<<22);  /* enable flag */
            msr_write(cpu_id, MSR_AMD_PERFEVTSEL2, flags);
            flags = msr_read(cpu_id, MSR_AMD_PERFEVTSEL3);
            flags |= (1<<22);  /* enable flag */
            msr_write(cpu_id, MSR_AMD_PERFEVTSEL3, flags);
            break;

        default:
            ERROR_MSG(Unsupported Processor);
            break;
    }

    timer_startCycles(&likwid_time[thread_id]);
}

    int
likwid_markerRegisterRegion(char* regionTag)
{
    static int lastRegion = -1;
    bstring tag = bfromcstr(regionTag);

    lastRegion++;

    if ( lastRegion >= likwid_numberOfRegions)
    {
        ERROR_PMSG(Too many regions %d,lastRegion);
    }

    likwid_results[lastRegion].tag = bstrcpy(tag);
    return lastRegion;
}

    int
likwid_markerGetRegionId(char* regionTag)
{
    bstring tag = bfromcstr(regionTag);

    for (int i=0; i<likwid_numberOfRegions;i++)
    {
        if (biseq(likwid_results[i].tag,tag))
        {
            return i;
        }
    }
    WARNING(Region not found);

    return -1;
}


void
likwid_markerStopRegion(int thread_id, int cpu_id, int regionId)
{
    uint64_t flags;

    timer_stopCycles(&likwid_time[thread_id]);
    likwid_results[regionId].time[thread_id] += timer_printCyclesTime(&likwid_time[thread_id]);

    switch ( cpuid_info.family ) 
    {
        case P6_FAMILY:

            switch ( cpuid_info.model ) 
            {
                case PENTIUM_M_BANIAS:
                    break;

                case PENTIUM_M_DOTHAN:
                    break;

                case CORE_DUO:
                    break;

                case XEON_MP:

                case CORE2_65:

                case CORE2_45:

                    msr_write(cpu_id, MSR_PERF_GLOBAL_CTRL, 0x0ULL);
                    likwid_results[regionId].counters[thread_id][0] += (double) msr_read(cpu_id, MSR_PERF_FIXED_CTR0);
                    likwid_results[regionId].counters[thread_id][1] += (double) msr_read(cpu_id, MSR_PERF_FIXED_CTR1);
                    likwid_results[regionId].counters[thread_id][2] += (double) msr_read(cpu_id, MSR_PMC0);
                    likwid_results[regionId].counters[thread_id][3] += (double) msr_read(cpu_id, MSR_PMC1);
                    break;

                case NEHALEM_WESTMERE_M:

                case NEHALEM_WESTMERE:

                case NEHALEM_BLOOMFIELD:

                case NEHALEM_LYNNFIELD:

                    msr_write(cpu_id, MSR_PERF_GLOBAL_CTRL, 0x0ULL);
                    likwid_results[regionId].counters[thread_id][0] += (double) msr_read(cpu_id, MSR_PERF_FIXED_CTR0);
                    likwid_results[regionId].counters[thread_id][1] += (double) msr_read(cpu_id, MSR_PERF_FIXED_CTR1);
                    likwid_results[regionId].counters[thread_id][2] += (double) msr_read(cpu_id, MSR_PERF_FIXED_CTR2);
                    likwid_results[regionId].counters[thread_id][3] += (double) msr_read(cpu_id, MSR_PMC0);
                    likwid_results[regionId].counters[thread_id][4] += (double) msr_read(cpu_id, MSR_PMC1);
                    likwid_results[regionId].counters[thread_id][5] += (double) msr_read(cpu_id, MSR_PMC2);
                    likwid_results[regionId].counters[thread_id][6] += (double) msr_read(cpu_id, MSR_PMC3);

                    if (nehalem_socket_lock[nehalem_processor_lookup[cpu_id]])
                    {
                        nehalem_socket_lock[nehalem_processor_lookup[cpu_id]] = 0;
                        msr_write(cpu_id, MSR_UNCORE_PERF_GLOBAL_CTRL, 0x0ULL);
                        likwid_results[regionId].counters[thread_id][7] += (double) msr_read(cpu_id, MSR_UNCORE_PMC0);
                        likwid_results[regionId].counters[thread_id][8] += (double) msr_read(cpu_id, MSR_UNCORE_PMC1);
                        likwid_results[regionId].counters[thread_id][9] += (double) msr_read(cpu_id, MSR_UNCORE_PMC2);
                        likwid_results[regionId].counters[thread_id][10] += (double) msr_read(cpu_id, MSR_UNCORE_PMC3);
                        likwid_results[regionId].counters[thread_id][11] += (double) msr_read(cpu_id, MSR_UNCORE_PMC4);
                        likwid_results[regionId].counters[thread_id][12] += (double) msr_read(cpu_id, MSR_UNCORE_PMC5);
                        likwid_results[regionId].counters[thread_id][13] += (double) msr_read(cpu_id, MSR_UNCORE_PMC6);
                        likwid_results[regionId].counters[thread_id][14] += (double) msr_read(cpu_id, MSR_UNCORE_PMC7);
                    }
                    break;

                case NEHALEM_EX:

                    msr_write(cpu_id, MSR_PERF_GLOBAL_CTRL, 0x0ULL);
                    likwid_results[regionId].counters[thread_id][0] += (double) msr_read(cpu_id, MSR_PERF_FIXED_CTR0);
                    likwid_results[regionId].counters[thread_id][1] += (double) msr_read(cpu_id, MSR_PERF_FIXED_CTR1);
                    likwid_results[regionId].counters[thread_id][2] += (double) msr_read(cpu_id, MSR_PERF_FIXED_CTR2);
                    likwid_results[regionId].counters[thread_id][3] += (double) msr_read(cpu_id, MSR_PMC0);
                    likwid_results[regionId].counters[thread_id][4] += (double) msr_read(cpu_id, MSR_PMC1);
                    likwid_results[regionId].counters[thread_id][5] += (double) msr_read(cpu_id, MSR_PMC2);
                    likwid_results[regionId].counters[thread_id][6] += (double) msr_read(cpu_id, MSR_PMC3);
                    break;

                default:
                    ERROR_MSG(Unsupported Processor);
                    break;
            }
            break;

        case K8_FAMILY:

        case K10_FAMILY:

            flags = msr_read(cpu_id,MSR_AMD_PERFEVTSEL0);
            flags &= ~(1<<22);  /* clear enable flag */
            msr_write(cpu_id, MSR_AMD_PERFEVTSEL0 , flags);
            flags = msr_read(cpu_id,MSR_AMD_PERFEVTSEL1);
            flags &= ~(1<<22);  /* clear enable flag */
            msr_write(cpu_id, MSR_AMD_PERFEVTSEL1 , flags);
            flags = msr_read(cpu_id,MSR_AMD_PERFEVTSEL2);
            flags &= ~(1<<22);  /* clear enable flag */
            msr_write(cpu_id, MSR_AMD_PERFEVTSEL2 , flags);
            flags = msr_read(cpu_id,MSR_AMD_PERFEVTSEL3);
            flags &= ~(1<<22);  /* clear enable flag */
            msr_write(cpu_id, MSR_AMD_PERFEVTSEL3 , flags);

            likwid_results[regionId].counters[cpu_id][0] += (double) msr_read(cpu_id, MSR_AMD_PMC0);
            likwid_results[regionId].counters[cpu_id][1] += (double) msr_read(cpu_id, MSR_AMD_PMC1);
            likwid_results[regionId].counters[cpu_id][2] += (double) msr_read(cpu_id, MSR_AMD_PMC2);
            likwid_results[regionId].counters[cpu_id][3] += (double) msr_read(cpu_id, MSR_AMD_PMC3);
            break;

        default:
            ERROR_MSG(Unsupported Processor);
            break;
    }
}

int  likwid_processGetProcessorId()
{
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    sched_getaffinity(getpid(),sizeof(cpu_set_t), &cpu_set);

    return getProcessorID(&cpu_set);
}


int  likwid_threadGetProcessorId()
{
    cpu_set_t  cpu_set;
    CPU_ZERO(&cpu_set);
    sched_getaffinity(gettid(),sizeof(cpu_set_t), &cpu_set);

    return getProcessorID(&cpu_set);
}


int  likwid_pinThread(int processorId)
{
    int ret;
    cpu_set_t cpuset;
    pthread_t thread;

    thread = pthread_self();
    CPU_ZERO(&cpuset);
    CPU_SET(processorId, &cpuset);
    ret = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);

    if (ret != 0)
    {   
        ERROR;
        return FALSE;
    }   

    return TRUE;
}


int  likwid_pinProcess(int processorId)
{
    int ret;
    cpu_set_t cpuset;

    CPU_ZERO(&cpuset);
    CPU_SET(processorId, &cpuset);
    ret = sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);

    if (ret < 0)
    {
        ERROR;
        return FALSE;
    }   

    return TRUE;
}


