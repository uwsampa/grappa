/*
 * ===========================================================================
 *
 *      Filename:  affinity.c
 *
 *      Description:  Implementation of affinity module.
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
#include <string.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sched.h>
#include <time.h>
#include <pthread.h>

#include <error.h>
#include <types.h>
#include <affinity.h>
#include <cpuid.h>
#include <tree.h>

/* #####   EXPORTED VARIABLES   ########################################### */


/* #####   MACROS  -  LOCAL TO THIS SOURCE FILE   ######################### */

#define gettid() syscall(SYS_gettid)


/* #####   VARIABLES  -  LOCAL TO THIS SOURCE FILE   ###################### */

static int  numberOfDomains = 1; /* add node domain per default */
static AffinityDomain*  domains;

/* #####   FUNCTION DEFINITIONS  -  LOCAL TO THIS SOURCE FILE   ########### */

static int
getProcessorID(cpu_set_t* cpu_set)
{
    int processorId;

    for (processorId=0;processorId<MAX_NUM_THREADS;processorId++)
    {
        if (CPU_ISSET(processorId,cpu_set))
        {
            break;
        }
    }
    return processorId;
}

/* this routine expects a core node and will traverse the tree over all threads
 * with physical cores first order */
static void
treeFillNextEntries(int numberOfEntries, int* processorIds, TreeNode** tree)
{
    int tmplist[numberOfEntries];
    int mapping[numberOfEntries];
    int counter = numberOfEntries;
    TreeNode* threadNode;
    int threadsPerCore = cpuid_topology.numThreadsPerCore;
    int row, column;
    int numberOfCores = numberOfEntries/threadsPerCore;

    while (*tree != NULL)
    {
        if (!counter) break;
        threadNode = tree_getChildNode(*tree);
        while (threadNode != NULL)
        {
            tmplist[numberOfEntries-counter] = threadNode->id;
            threadNode = tree_getNextNode(threadNode);
            counter--;
        }
        *tree = tree_getNextNode(*tree);
    }

    for (int i=0; i<numberOfEntries; i++)
    {
        column = i%threadsPerCore;
        row = i/threadsPerCore;
        mapping[column*numberOfCores+row]=i;
    }

    for (int i=0; i<numberOfEntries; i++)
    {
        processorIds[i] = tmplist[mapping[i]];
    }

}

static void
treeFillEntriesNode(int* processorIds)
{
    uint32_t i;
    int tmplist[cpuid_topology.numHWThreads];
    int mapping[cpuid_topology.numHWThreads];
    int counter = cpuid_topology.numHWThreads;
    TreeNode* threadNode;
    TreeNode* coreNode;
    TreeNode* socketNode;
    int threadsPerCore = cpuid_topology.numThreadsPerCore;
    int row, column;
    int numberOfCores = cpuid_topology.numHWThreads/threadsPerCore;

    socketNode = tree_getChildNode(cpuid_topology.topologyTree);
    while (socketNode != NULL)
    {
        coreNode = tree_getChildNode(socketNode);

        while (coreNode != NULL)
        {
            threadNode = tree_getChildNode(coreNode);
            while (threadNode != NULL)
            {
                tmplist[cpuid_topology.numHWThreads-counter] = threadNode->id;
                threadNode = tree_getNextNode(threadNode);
                counter--;
            }
            coreNode = tree_getNextNode(coreNode);
        }
        socketNode = tree_getNextNode(socketNode);
    }

    for (i=0; i< cpuid_topology.numHWThreads; i++)
    {
        column = i%threadsPerCore;
        row = i/threadsPerCore;
        mapping[column*numberOfCores+row]=i;
    }

    for (i=0; i< cpuid_topology.numHWThreads; i++)
    {
        processorIds[i] = tmplist[mapping[i]];
    }

}


/* #####   FUNCTION DEFINITIONS  -  EXPORTED FUNCTIONS   ################## */

void affinity_init()
{
    int i;
    int cacheDomain;
    int currentDomain;
    TreeNode* socketNode;
    TreeNode* coreNode;

    int numberOfCoresPerCache =
        cpuid_topology.cacheLevels[cpuid_topology.numCacheLevels-1].threads/cpuid_topology.numThreadsPerCore;
    int numberOfProcessorsPerCache =
        cpuid_topology.cacheLevels[cpuid_topology.numCacheLevels-1].threads;

    /* determine total number of domains */
    numberOfDomains += cpuid_topology.numSockets;

    /* for the cache domain take only into account last level cache and assume
     * all sockets to be uniform.
     * */

    /* determine how many last level shared caches exist per socket */
 //   numberOfDomains += cpuid_topology.numSockets * (cpuid_topology.numCoresPerSocket/numberOfCoresPerCache);
    numberOfDomains += cpuid_topology.numSockets * (cpuid_topology.numCoresPerSocket/numberOfCoresPerCache);

     /* :TODO:05/01/2010 10:02:36 AM:jt: Add NUMA domains here */

    domains = (AffinityDomain*) malloc(numberOfDomains * sizeof(AffinityDomain));
    cacheDomain = 0;

    /* Node domain */
    domains[0].numberOfProcessors = cpuid_topology.numHWThreads;
    domains[0].processorList = (int*) malloc(cpuid_topology.numHWThreads*sizeof(int));
    domains[0].tag = bformat("N");

    for (i=1; i<numberOfDomains; i++) {
        if (i < (int) (cpuid_topology.numSockets+1))
        {
            domains[i].numberOfProcessors = cpuid_topology.numCoresPerSocket *
                cpuid_topology.numThreadsPerCore;
            domains[i].processorList = (int*) malloc(cpuid_topology.numCoresPerSocket*
                    cpuid_topology.numThreadsPerCore * sizeof(int));
            domains[i].tag = bformat("S%d",i-1);
        }
        else
        {
            domains[i].processorList = (int*)
                malloc(numberOfProcessorsPerCache*sizeof(int));
            domains[i].tag = bformat("C%d",cacheDomain++);
            domains[i].numberOfProcessors = numberOfProcessorsPerCache;
        }
    }

    treeFillEntriesNode(domains[0].processorList);
    currentDomain = 0;
    /* create socket domains */
    socketNode = tree_getChildNode(cpuid_topology.topologyTree);

    while (socketNode != NULL)
    {
        currentDomain++;
        coreNode = tree_getChildNode(socketNode);
        treeFillNextEntries(domains[currentDomain].numberOfProcessors, domains[currentDomain].processorList, &coreNode);
        socketNode = tree_getNextNode(socketNode);
    }

    /* create last level cache domains */
    socketNode = tree_getChildNode(cpuid_topology.topologyTree);
    while (socketNode != NULL)
    {
        coreNode = tree_getChildNode(socketNode);
        for (i=0; i< (int) (cpuid_topology.numCoresPerSocket/numberOfCoresPerCache); i++)
        {
            currentDomain++;
            treeFillNextEntries(domains[currentDomain].numberOfProcessors, domains[currentDomain].processorList, &coreNode);
        }

        socketNode = tree_getNextNode(socketNode);
    }
}


void affinity_finalize()
{
    int i;

    for (i=0; i<numberOfDomains; i++) {
        free(domains[i].processorList);
    }
    free(domains);
}


int  affinity_processGetProcessorId()
{
    int ret;
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    ret = sched_getaffinity(getpid(),sizeof(cpu_set_t), &cpu_set);

    if (ret < 0)
    {
        ERROR;
    }

    return getProcessorID(&cpu_set);
}


int  affinity_threadGetProcessorId()
{
    cpu_set_t  cpu_set;
    CPU_ZERO(&cpu_set);
    sched_getaffinity(gettid(),sizeof(cpu_set_t), &cpu_set);

    return getProcessorID(&cpu_set);
}


void  affinity_pinThread(int processorId)
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
    }
}


void  affinity_pinProcess(int processorId)
{
    int ret;
	cpu_set_t cpuset;

	CPU_ZERO(&cpuset);
	CPU_SET(processorId, &cpuset);
	ret = sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);

	if (ret < 0) 
	{
        ERROR;
	}
}


const AffinityDomain* affinity_getDomain(bstring domain)
{
    int i;

    for (i=0; i<numberOfDomains; i++)
    {
        if (biseq(domain, domains[i].tag))
        {
            return domains+i;
        }
    }

    return NULL;
}

void affinity_printDomains()
{
    int i;
    uint32_t j;

    for (i=0; i<numberOfDomains; i++)
    {
        printf("Domain %d:\n",i);
        printf("\tTag %s:",bdata(domains[i].tag));
        for (j=0; j<domains[i].numberOfProcessors; j++)
        {
            printf(" %d",domains[i].processorList[j]);
        }
        printf("\n");
    }
}



