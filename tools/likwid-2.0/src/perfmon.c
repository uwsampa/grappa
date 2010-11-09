/*
 * ===========================================================================
 *
 *      Filename:  perfmon.c
 *
 *      Description:  Implementation of perfmon Module.
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
#include <math.h>
#include <float.h>

#include <bstrlib.h>
#include <strUtil.h>
#include <error.h>
#include <types.h>
#include <timer.h>
#include <msr.h>
#include <cpuid.h>
#include <perfmon.h>
#include <asciiTable.h>
#include <registers.h>


/* #####   EXPORTED VARIABLES   ########################################### */

int perfmon_numArchEvents;
int perfmon_numThreads;
int perfmon_numRegions;
int perfmon_verbose=0;
PerfmonThread* perfmon_threadData;
double** perfmon_threadState;

/* #####   VARIABLES  -  LOCAL TO THIS SOURCE FILE   ###################### */

static PerfmonGroup groupSet = NOGROUP;
static PerfmonEvent* eventHash;
static PerfmonCounterMap* counter_map;
static PerfmonGroupMap* group_map;
static PerfmonGroupHelp* group_help;

static CyclesData timeData;
static double rdtscTime;
static PerfmonEventSet perfmon_set;
static int perfmon_numGroups;
static int perfmon_numCounters;

/* #####   PROTOTYPES  -  LOCAL TO THIS SOURCE FILE   ##################### */

static void initResultTable(PerfmonResultTable* tableData,
        bstrList* firstColumn,
        int numRows,
        int numColumns);

static void initStatisticTable(PerfmonResultTable* tableData,
        bstrList* firstColumn,
        int numRows);

static void printResultTable(PerfmonResultTable* tableData);
static void freeResultTable(PerfmonResultTable* tableData);
static void initThread(int , int );

/* #####   MACROS  -  LOCAL TO THIS SOURCE FILE   ######################### */

#define CHECKERROR \
        if (ret == EOF) \
        { \
            fprintf (stderr, "sscanf: Failed to read marker file!\n" ); \
            exit (EXIT_FAILURE);}

#define bstrListAdd(id,name) \
    label = bfromcstr(#name);  \
    fc->entry[id] = bstrcpy(label);  \
    fc->qty++; \
    bdestroy(label);

#define INIT_EVENTS   \
    fc = bstrListCreate(); \
    bstrListAlloc(fc, numRows+1); \
    bstrListAdd(0,Event); \
    for (i=0; i<numRows; i++) \
    { \
        fc->entry[1+i] = \
           bfromcstr(perfmon_set.events[i].event.name); } 

#define INIT_BASIC  \
    fc = bstrListCreate(); \
    bstrListAlloc(fc, numRows+1); \
    bstrListAdd(0,Metric);

#include <perfmon_pm.h>
#include <perfmon_atom.h>
#include <perfmon_core2.h>
#include <perfmon_nehalem.h>
#include <perfmon_nehalemEX.h>
#include <perfmon_westmere.h>
#include <perfmon_k8.h>
#include <perfmon_k10.h>

/* #####  EXPORTED  FUNCTION POINTERS   ################################### */
void (*perfmon_startCountersThread) (int thread_id);
void (*perfmon_stopCountersThread) (int thread_id);
void (*perfmon_readCountersThread) (int thread_id);
void (*perfmon_setupCounterThread) (int thread_id,
        uint32_t umask, uint32_t event, PerfmonCounterIndex index);
void (*printDerivedMetrics) (PerfmonGroup group);
void (*logDerivedMetrics) (PerfmonGroup group, double time, double timeStamp);

/* #####   FUNCTION POINTERS  -  LOCAL TO THIS SOURCE FILE ################ */

static void (*initThreadArch) (PerfmonThread *thread);

/* #####   FUNCTION DEFINITIONS  -  LOCAL TO THIS SOURCE FILE   ########### */

static int  getIndex (bstring reg, PerfmonCounterIndex* index)
{
    int i;

    for (i=0; i< perfmon_numCounters; i++)
    {
        if (biseqcstr(reg, counter_map[i].key))
        {
            *index = counter_map[i].index;
            return TRUE;
        }
    }

    return FALSE;
}

static int
getEvent(bstring event_str, PerfmonEvent* event)
{
    int i;

    for (i=0; i< perfmon_numArchEvents; i++)
    {
        if (biseqcstr(event_str, eventHash[i].name))
        {
            *event = eventHash[i];

            if (perfmon_verbose)
            {
                printf ("Found event %s : Event_id 0x%02X Umask 0x%02X \n",
                        bdata(event_str), event->eventId, event->umask);
            }
            return TRUE;
        }
    }

    return FALSE;
}

static void
initThread(int thread_id, int cpu_id)
{
    int i;

    for (i=0; i<NUM_PMC; i++)
    {
        perfmon_threadData[thread_id].counters[i].init = FALSE;
    }

    perfmon_threadData[thread_id].processorId = cpu_id;
    initThreadArch(&perfmon_threadData[thread_id]);

}

struct cbsScan{
	/* Parse state */
	bstring src;
	int line;
    LikwidResults* results;
};

static int lineCb (void* parm, int ofs, int len)
{
    int ret;
    struct cbsScan* st = (struct cbsScan*) parm;
    struct bstrList* strList;
    bstring line;

    if (!len) return 1;
    strList = bstrListCreate();

    line = blk2bstr (st->src->data + ofs, len);

    if (st->line < perfmon_numRegions)
    {
        int id;
        strList = bsplit(line,':');

        if( strList->qty < 2 )
        {
            ERROR_MSG(Failed to read marker file);
        }
        ret = sscanf (bdata(strList->entry[0]), "%d", &id); CHECKERROR;
        st->results[id].tag = bstrcpy(strList->entry[1]);
    }
    else
    {
        int tagId;
        int threadId;

        strList = bsplit(line,32);

        if( strList->qty < (3+NUM_PMC))
        {
            ERROR_MSG(Failed to read marker file);
        }

        ret = sscanf(bdata(strList->entry[0]), "%d", &tagId); CHECKERROR;
        ret = sscanf(bdata(strList->entry[1]), "%d", &threadId); CHECKERROR;
        ret = sscanf(bdata(strList->entry[2]), "%lf", &st->results[tagId].time[threadId]); CHECKERROR;

        for (int i=0;i<NUM_PMC; i++)
        {
            ret = sscanf(bdata(strList->entry[3+i]), "%lf", &st->results[tagId].counters[threadId][i]); CHECKERROR;
        }
    }

    bstrListDestroy(strList);
    st->line++;
    bdestroy(line);
    return 1;
}

static void
readMarkerFile(bstring filename, LikwidResults** resultsRef)
{
	int numberOfThreads=0;
	int ret;
	int i,j,k;
    struct cbsScan sl;
	FILE * fp;
    LikwidResults* results = *resultsRef;

	if (NULL != (fp = fopen (bdata(filename), "r"))) 
	{
		bstring src = bread ((bNread) fread, fp);

		/* read header info */
		ret = sscanf (bdata(src), "%d %d", &numberOfThreads, &perfmon_numRegions); CHECKERROR;
		results = (LikwidResults*) malloc(perfmon_numRegions * sizeof(LikwidResults));

        if (numberOfThreads != perfmon_numThreads)
        {
            printf("ERROR: Is the number of threads for likwid-perfCtr equal to the number in the measured application?\n");
            printf("likwid_markerInit and likwid_markerRegisterRegion must be called in serial region.\n");
            ERROR_MSG(Number of threads in marker file unequal to number of threads in likwid-perfCtr);
        }

		/* allocate  LikwidResults struct */
		for (i=0;i<perfmon_numRegions; i++)
		{
			results[i].time = (double*) malloc(numberOfThreads * sizeof(double));
			results[i].counters = (double**) malloc(numberOfThreads * sizeof(double*));

			for (j=0;j<numberOfThreads; j++)
			{
				results[i].time[j] = 0.0;
				results[i].counters[j] = (double*) malloc(NUM_PMC * sizeof(double));
				for (k=0;k<NUM_PMC; k++)
				{
					results[i].counters[j][k] = 0.0;
				}
			}
		}

        sl.src = src;
        sl.line = 0;
        sl.results = results;
        bsplitcb (src, (char) '\n', bstrchr(src,10)+1, lineCb, &sl);

		fclose (fp);
		bdestroy (src);
	}
    else
    {
        printf("ERROR: The marker result file could not be found!\n");
        printf("Did you call likwid_markerClose() at the end of your measurement?\n");
        ERROR;
    }

    *resultsRef = results;

    bstring exeString = bformat("rm  -f %s",bdata(filename));

    ret = system(bdata(exeString));

    if (ret == EOF)
    {
        ERROR;
    }

    bdestroy(exeString);
}

static void
printResultTable(PerfmonResultTable* tableData)
{
    int i,j;
    TableContainer* table;
    bstrList* labelStrings = NULL;
    bstring label = bfromcstr("NO");

    table = asciiTable_allocate(tableData->numRows,
            tableData->numColumns+1,
            tableData->header);

    labelStrings = bstrListCreate();
    bstrListAlloc(labelStrings, tableData->numColumns+1);

    for (i=0; i<tableData->numRows; i++)
    {
        labelStrings->qty = 0;
        labelStrings->entry[0] = bstrcpy(tableData->rows[i].label);
        labelStrings->qty++;

        for (j=0; j<(tableData->numColumns);j++)
        {
            if (!isnan(tableData->rows[i].value[j]))
            {
                label = bformat("%g", tableData->rows[i].value[j]);
            }
            else
            {
                label = bformat("0");
            }

            labelStrings->entry[1+j] = bstrcpy(label);
            labelStrings->qty++;
        }
        asciiTable_appendRow(table,labelStrings);
    }

    asciiTable_print(table);
    bdestroy(label);
    bstrListDestroy(labelStrings);
    asciiTable_free(table);
}

static int
getGroupId(bstring groupStr,PerfmonGroup* group)
{
    *group = NOGROUP;

    for (int i=0; i<perfmon_numGroups; i++)
    {
        if (biseqcstr(groupStr,group_map[i].key)) 
        {
            *group = group_map[i].index;
            return i;
        }
    }

    return -1;
}

static int
checkCounter(bstring counterName, char* limit)
{
    int i;
    struct bstrList* tokens;
    int value = FALSE;
    bstring limitString = bfromcstr(limit);

    tokens = bstrListCreate();
    tokens = bsplit(limitString,'|');

    for(i=0; i<tokens->qty; i++)
    {
        if(bstrncmp(counterName, tokens->entry[i], blength(tokens->entry[i])))
        {
            value = FALSE;
        }
        else
        {
            value = TRUE;
            break;
        }
    }

    bdestroy(limitString);
    bstrListDestroy(tokens);
    return value;
}

static void
freeResultTable(PerfmonResultTable* tableData)
{
    int i;

    bstrListDestroy(tableData->header);

    for (i=0; i<tableData->numRows; i++)
    {
        free(tableData->rows[i].value);
    }

    free(tableData->rows);
}

static void 
initResultTable(PerfmonResultTable* tableData,
        bstrList* firstColumn,
        int numRows,
        int numColumns)
{
    int i;
    bstrList* header;
    bstring label;

    header = bstrListCreate();
    bstrListAlloc(header, numColumns+1);
    header->entry[0] = bstrcpy(firstColumn->entry[0]); header->qty++;

    for (i=0; i<perfmon_numThreads;i++)
    {
        label = bformat("core %d",perfmon_threadData[i].processorId);
        header->entry[1+i] = bstrcpy(label); header->qty++;
    }

    tableData->numRows = numRows;
    tableData->numColumns = numColumns;
    tableData->header = header;
    tableData->rows = (PerfmonResult*) malloc(numRows*sizeof(PerfmonResult));

    for (i=0; i<numRows; i++)
    {
//        tableData->rows[i].label =
//           bfromcstr(perfmon_set.events[i].event.name);

        tableData->rows[i].label = firstColumn->entry[1+i];

        tableData->rows[i].value =
            (double*) malloc((numColumns)*sizeof(double));
    }
}

static void 
initStatisticTable(PerfmonResultTable* tableData,
        bstrList* firstColumn,
        int numRows)
{
    int i;
    int numColumns = 4;
    bstrList* header;
    bstring label;

    header = bstrListCreate();
    bstrListAlloc(header, numColumns+1);
    header->entry[0] = bstrcpy(firstColumn->entry[0]); header->qty++;

    label = bformat("Sum");
    header->entry[1] = bstrcpy(label); header->qty++;
    label = bformat("Max");
    header->entry[2] = bstrcpy(label); header->qty++;
    label = bformat("Min");
    header->entry[3] = bstrcpy(label); header->qty++;
    label = bformat("Avg");
    header->entry[4] = bstrcpy(label); header->qty++;

    tableData->numRows = numRows;
    tableData->numColumns = numColumns;
    tableData->header = header;
    tableData->rows = (PerfmonResult*) malloc(numRows*sizeof(PerfmonResult));

    for (i=0; i<numRows; i++)
    {
//        tableData->rows[i].label =
//           bfromcstr(perfmon_set.events[i].event.name);

        tableData->rows[i].label = firstColumn->entry[1+i];
        bcatcstr(tableData->rows[i].label," STAT");

        tableData->rows[i].value =
            (double*) malloc((numColumns)*sizeof(double));
    }
}


/* #####   FUNCTION DEFINITIONS  -  EXPORTED FUNCTIONS   ################## */

void
perfmon_printCounters(void)
{
    int i;

    printf("This architecture has %d counters.\n", perfmon_numCounters);
    printf("Counters names:  ");

    for (i=0; i<perfmon_numCounters; i++)
    {
        printf("%s\t",counter_map[i].key);
    }
    printf(".\n");
}

void
perfmon_printEvents(void)
{
    int i;

    printf("This architecture has %d events.\n", perfmon_numArchEvents);
    printf("Event tags (tag, id, umask, counters):\n");

    for (i=0; i<perfmon_numArchEvents; i++)
    {
        printf("%s, 0x%X, 0x%X, %s \n",
                eventHash[i].name,
                eventHash[i].eventId,
                eventHash[i].umask,
                eventHash[i].limit);
    }
}


double
perfmon_getResult(int threadId, char* counterString)
{
    bstring counter = bfromcstr(counterString);
    PerfmonCounterIndex  index;

   if (getIndex(counter,&index))
   {
           return perfmon_threadData[threadId].counters[index].counterData;
   }

   fprintf (stderr, "perfmon_getResult: Failed to get counter Index!\n" );
   return 0.0;
}


void
perfmon_initEventSet(StrUtilEventSet* eventSetConfig, PerfmonEventSet* set)
{
    int i;

    set->numberOfEvents = eventSetConfig->numberOfEvents;
    set->events = (PerfmonEventSetEntry*)
        malloc(set->numberOfEvents * sizeof(PerfmonEventSetEntry));

    for (i=0; i<set->numberOfEvents; i++)
    {
        /* get register index */
        if (!getIndex(eventSetConfig->events[i].counterName,
                    &set->events[i].index))
        {
            ERROR_PMSG(Counter register %s not supported,bdata(eventSetConfig->events[i].counterName));
        }

        /* setup event */
        if (!getEvent(eventSetConfig->events[i].eventName,
                    &set->events[i].event))
        {
            ERROR_PMSG(Event %s not found for current architecture,bdata(eventSetConfig->events[i].eventName));
        }
        
        /* is counter allowed for event */
        if (!checkCounter(eventSetConfig->events[i].counterName,
                    set->events[i].event.limit))
        {
            ERROR_PMSG(Register not allowed  for event  %s, bdata(eventSetConfig->events[i].eventName));
        }
    }
}

void 
perfmon_printMarkerResults(bstring filepath)
{
    int i;
    int j;
    int region;
    LikwidResults* results = NULL;
    PerfmonResultTable tableData;
    int numRows = perfmon_set.numberOfEvents;
    int numColumns = perfmon_numThreads;
    bstrList* fc;
    bstring label;
    INIT_EVENTS;

    readMarkerFile(filepath, &results);
    initResultTable(&tableData, fc, numRows, numColumns);

    for (region=0; region<perfmon_numRegions; region++)
    {
        printf("Region: %s \n",bdata(results[region].tag));

        for (i=0; i<numRows; i++)
        {
            for (j=0; j<numColumns; j++)
            {
                tableData.rows[i].value[j] =
                    results[region].counters[j][perfmon_set.events[i].index];
            }
        }
        printResultTable(&tableData);


        for (j=0; j<numColumns; j++)
        {
            for (i=0; i<numRows; i++)
            {
                perfmon_threadData[j].counters[perfmon_set.events[i].index].counterData =
                    results[region].counters[j][perfmon_set.events[i].index];
            }
        }
        rdtscTime = results[region].time[0];
        printf("RDTSC timing [thread 0]: %g s\n",rdtscTime);
        printDerivedMetrics(groupSet);
    }

    for (i=0;i<perfmon_numRegions; i++)
    {
        for (j=0;j<perfmon_numThreads; j++)
        {
            free(results[i].counters[j]);
        }

        free(results[i].counters);
        free(results[i].time);
    }

    freeResultTable(&tableData);
}

void 
perfmon_logCounterResults(double time)
{
    int i;
    int j;
    double tmp;
    static double timeStamp = 0.0;

    timeStamp += time;

    for (i=0; i<perfmon_set.numberOfEvents; i++)
    {
        printf("%s %e ",perfmon_set.events[i].event.name, timeStamp);
        for (j=0; j<perfmon_numThreads; j++)
        {
            printf("%e ",
                    (double) (perfmon_threadData[j].counters[perfmon_set.events[i].index].counterData) - perfmon_threadState[j][i]) ;
            tmp =perfmon_threadData[j].counters[perfmon_set.events[i].index].counterData ; 
            perfmon_threadData[j].counters[perfmon_set.events[i].index].counterData -= perfmon_threadState[j][i] ;
            perfmon_threadState[j][i] = tmp;
        }
        printf("\n");
    }
    logDerivedMetrics(groupSet, time, timeStamp);
}

void 
perfmon_printCounterResults()
{
    int i;
    int j;
    PerfmonResultTable tableData;
    int numRows = perfmon_set.numberOfEvents;
    int numColumns = perfmon_numThreads;
    double stat[perfmon_set.numberOfEvents][4]; /* 0:sum, 1:max, 2:min, 3:avg */
    bstrList* fc;
    bstring label;
    INIT_EVENTS;

    for (i=0; i<numRows; i++)
    {
        stat[i][0] = 0;
        stat[i][1] = 0;
        stat[i][2] = DBL_MAX;
    }

    initResultTable(&tableData, fc, numRows, numColumns);

    /* print raw event data */
    for (i=0; i<numRows; i++)
    {
        for (j=0; j<numColumns; j++)
        {
            tableData.rows[i].value[j] =
                (double) perfmon_threadData[j].counters[perfmon_set.events[i].index].counterData;
            stat[i][0] += (double) perfmon_threadData[j].counters[perfmon_set.events[i].index].counterData;
            stat[i][1] =  MAX(stat[i][1],(double) perfmon_threadData[j].counters[perfmon_set.events[i].index].counterData);
            stat[i][2] =  MIN(stat[i][2],(double) perfmon_threadData[j].counters[perfmon_set.events[i].index].counterData);
        }
    }
    printResultTable(&tableData);
    freeResultTable(&tableData);


    /* for threaded results print sum, max, min and avg */
    if (perfmon_numThreads > 1)
    {
        initStatisticTable(&tableData, fc, numRows);
        for (i=0; i<numRows; i++)
        {
            stat[i][3] =  stat[i][0]/perfmon_numThreads;
            for (j=0; j<4; j++)
            {
                tableData.rows[i].value[j] = stat[i][j];
            }
        }
        printResultTable(&tableData);
        freeResultTable(&tableData);
    }

    /* print derived metrics */
    printDerivedMetrics(groupSet);
}


void
perfmon_setupEventSetC(char* eventCString)
{
    bstring eventString = bfromcstr(eventCString);
    StrUtilEventSet eventSetConfig;

    groupSet = NOGROUP;
    bstr_to_eventset(&eventSetConfig, eventString);
    perfmon_initEventSet(&eventSetConfig, &perfmon_set);
    perfmon_setupCounters();
    bdestroy(eventString);
}

void
perfmon_setupEventSet(bstring eventString)
{
    int groupId;
    StrUtilEventSet eventSetConfig;

    groupId = getGroupId(eventString, &groupSet);

    if (groupSet == NOGROUP)
    {
        /* eventString is a custom eventSet */
        bstr_to_eventset(&eventSetConfig, eventString);
    }
    else
    {
        printf("Measuring group %s\n", group_map[groupId].key);
        /* eventString is a group */
        eventString = bfromcstr(group_map[groupId].config);
        bstr_to_eventset(&eventSetConfig, eventString);
    }

    perfmon_initEventSet(&eventSetConfig, &perfmon_set);
    perfmon_setupCounters();
}


void
perfmon_setupCounters()
{
  int i;
  int j;
  uint32_t eventId;
  uint32_t umask;
  PerfmonCounterIndex  index;

    for (j=0; j<perfmon_set.numberOfEvents; j++)
    {
        eventId = perfmon_set.events[j].event.eventId;
        umask   = perfmon_set.events[j].event.umask;
        index   = perfmon_set.events[j].index;
 //       printf("%s\n",perfmon_set.events[i].name);

        for (i=0; i<perfmon_numThreads; i++)
        {
            perfmon_setupCounterThread(i,
                    eventId,
                    umask,
                    index);
        }
    }
}

void
perfmon_startCounters(void)
{
    int i;

    for (i=0;i<perfmon_numThreads;i++)
    {
        perfmon_startCountersThread(i);
    }

    timer_startCycles(&timeData);
}

void
perfmon_stopCounters(void)
{
    int i;

    timer_stopCycles(&timeData);

    for (i=0;i<perfmon_numThreads;i++)
    {
        perfmon_stopCountersThread(i);
    }

    rdtscTime = timer_printCyclesTime(&timeData);
}

void
perfmon_readCounters(void)
{
    int i;

    for (i=0;i<perfmon_numThreads;i++)
    {
        perfmon_readCountersThread(i);
    }
}


void
perfmon_printAvailableGroups()
{
    int i;

    printf("Available groups on %s:\n",cpuid_info.name);

    for(i=0; i<perfmon_numGroups; i++)
    {
        printf("%s: %s\n",group_map[i].key,
                group_map[i].info);
    }
}

void
perfmon_printGroupHelp(bstring group)
{
    int i;
    PerfmonGroup groupDummy;

    if ((i = getGroupId(group,&groupDummy))<0)
    {
        ERROR_MSG(Group not found);
    }
    else
    {
        printf("Group %s:\n",bdata(group));
        printf("%s",group_help[i].msg);
    }
}



void
perfmon_init(int numThreads_local, int threads[])
{
    int i;
    perfmon_numThreads = numThreads_local;
    perfmon_threadData = (PerfmonThread*) malloc(perfmon_numThreads * sizeof(PerfmonThread));
    //This is specific for daemon mode.
    perfmon_threadState = (double**) malloc(perfmon_numThreads * sizeof(double*));
    for (i=0; i<perfmon_numThreads; i++)
    {
        perfmon_threadState[i] = (double*) malloc(NUM_PMC * sizeof(double));
    }

    switch ( cpuid_info.family ) 
    {
        case P6_FAMILY:

            switch ( cpuid_info.model ) 
            {
                case PENTIUM_M_BANIAS:

                case PENTIUM_M_DOTHAN:

                    eventHash = pm_arch_events;
                    perfmon_numArchEvents = perfmon_numArchEvents_pm;

                    group_map = pm_group_map;
                 //   group_help = pm_group_help;
                    perfmon_numGroups = perfmon_numGroups_pm;

                    counter_map = pm_counter_map;
                    perfmon_numCounters = perfmon_numCounters_pm;

                    initThreadArch = perfmon_init_pm;
                    printDerivedMetrics = perfmon_printDerivedMetrics_pm;
                    perfmon_startCountersThread = perfmon_startCountersThread_pm;
                    perfmon_stopCountersThread = perfmon_stopCountersThread_pm;
                    perfmon_setupCounterThread = perfmon_setupCounterThread_pm;
                    break;

                case ATOM:

                    eventHash = atom_arch_events;
                    perfmon_numArchEvents = perfmon_numArchEventsAtom;

                    group_map = atom_group_map;
                    group_help = atom_group_help;
                    perfmon_numGroups = perfmon_numGroupsAtom;

                    counter_map = core2_counter_map;
                    perfmon_numCounters = perfmon_numCountersCore2;

                    initThreadArch = perfmon_init_core2;
                    printDerivedMetrics = perfmon_printDerivedMetricsAtom;
                    perfmon_startCountersThread = perfmon_startCountersThread_core2;
                    perfmon_stopCountersThread = perfmon_stopCountersThread_core2;
                    perfmon_setupCounterThread = perfmon_setupCounterThread_core2;
                    break;


                case CORE_DUO:
                    ERROR_MSG(Unsupported Processor);
                    break;

                case XEON_MP:

                case CORE2_65:

                case CORE2_45:

                    eventHash = core2_arch_events;
                    perfmon_numArchEvents = perfmon_numArchEventsCore2;

                    group_map = core2_group_map;
                    group_help = core2_group_help;
                    perfmon_numGroups = perfmon_numGroupsCore2;

                    counter_map = core2_counter_map;
                    perfmon_numCounters = perfmon_numCountersCore2;

                    initThreadArch = perfmon_init_core2;
                    printDerivedMetrics = perfmon_printDerivedMetricsCore2;
                    logDerivedMetrics = perfmon_logDerivedMetricsCore2;
                    perfmon_startCountersThread = perfmon_startCountersThread_core2;
                    perfmon_stopCountersThread = perfmon_stopCountersThread_core2;
                    perfmon_readCountersThread = perfmon_readCountersThread_core2;
                    perfmon_setupCounterThread = perfmon_setupCounterThread_core2;
                    break;

                case NEHALEM_EX:

                    eventHash = nehalemEX_arch_events;
                    perfmon_numArchEvents = perfmon_numArchEventsNehalemEX;

                    group_map = nehalemEX_group_map;
                    group_help = nehalemEX_group_help;
                    perfmon_numGroups = perfmon_numGroupsNehalemEX;

                    counter_map = nehalemEX_counter_map;
                    perfmon_numCounters = perfmon_numCountersNehalemEX;

                    initThreadArch = perfmon_init_nehalemEX;
                    printDerivedMetrics = perfmon_printDerivedMetricsNehalemEX;
                    logDerivedMetrics = perfmon_logDerivedMetricsNehalemEX;
                    perfmon_startCountersThread = perfmon_startCountersThread_nehalemEX;
                    perfmon_stopCountersThread = perfmon_stopCountersThread_nehalemEX;
                    perfmon_readCountersThread = perfmon_readCountersThread_nehalemEX;
                    perfmon_setupCounterThread = perfmon_setupCounterThread_nehalemEX;
                    break;

                case NEHALEM_BLOOMFIELD:

                case NEHALEM_LYNNFIELD:

                    eventHash = nehalem_arch_events;
                    perfmon_numArchEvents = perfmon_numArchEventsNehalem;

                    group_map = nehalem_group_map;
                    group_help = nehalem_group_help;
                    perfmon_numGroups = perfmon_numGroupsNehalem;

                    counter_map = nehalem_counter_map;
                    perfmon_numCounters = perfmon_numCountersNehalem;

                    initThreadArch = perfmon_init_nehalem;
                    printDerivedMetrics = perfmon_printDerivedMetricsNehalem;
                    logDerivedMetrics = perfmon_logDerivedMetricsNehalem;
                    perfmon_startCountersThread = perfmon_startCountersThread_nehalem;
                    perfmon_stopCountersThread = perfmon_stopCountersThread_nehalem;
                    perfmon_readCountersThread = perfmon_readCountersThread_nehalem;
                    perfmon_setupCounterThread = perfmon_setupCounterThread_nehalem;
                    break;

                case NEHALEM_WESTMERE_M:
                    
                case NEHALEM_WESTMERE:

                    eventHash = westmere_arch_events;
                    perfmon_numArchEvents = perfmon_numArchEventsWestmere;

                    group_map = westmere_group_map;
                    group_help = westmere_group_help;
                    perfmon_numGroups = perfmon_numGroupsWestmere;

                    counter_map = nehalem_counter_map;
                    perfmon_numCounters = perfmon_numCountersNehalem;

                    initThreadArch = perfmon_init_nehalem;
                    printDerivedMetrics = perfmon_printDerivedMetricsWestmere;
                    logDerivedMetrics = perfmon_logDerivedMetricsWestmere;
                    perfmon_startCountersThread = perfmon_startCountersThread_nehalem;
                    perfmon_stopCountersThread = perfmon_stopCountersThread_nehalem;
                    perfmon_readCountersThread = perfmon_readCountersThread_nehalem;
                    perfmon_setupCounterThread = perfmon_setupCounterThread_nehalem;
                    break;

                default:
                    ERROR_MSG(Unsupported Processor);
                    break;
            }
            break;

        case K8_FAMILY:
            eventHash = k8_arch_events;
            perfmon_numArchEvents = perfmon_numArchEventsK8;

            group_map = k8_group_map;
            group_help = k8_group_help;
            perfmon_numGroups = perfmon_numGroupsK8;

            counter_map = k10_counter_map;
            perfmon_numCounters = perfmon_numCountersK10;

            initThreadArch = perfmon_init_k10;
            printDerivedMetrics = perfmon_printDerivedMetricsK10;
            logDerivedMetrics = perfmon_logDerivedMetricsK10;
            perfmon_startCountersThread = perfmon_startCountersThread_k10;
            perfmon_stopCountersThread = perfmon_stopCountersThread_k10;
            perfmon_readCountersThread = perfmon_readCountersThread_k10;
            perfmon_setupCounterThread = perfmon_setupCounterThread_k10;
            break;

        case K10_FAMILY:
            eventHash = k10_arch_events;
            perfmon_numArchEvents = perfmon_numArchEventsK10;

            group_map = k10_group_map;
            group_help = k10_group_help;
            perfmon_numGroups = perfmon_numGroupsK10;

            counter_map = k10_counter_map;
            perfmon_numCounters = perfmon_numCountersK10;

            initThreadArch = perfmon_init_k10;
            printDerivedMetrics = perfmon_printDerivedMetricsK10;
            logDerivedMetrics = perfmon_logDerivedMetricsK10;
            perfmon_startCountersThread = perfmon_startCountersThread_k10;
            perfmon_stopCountersThread = perfmon_stopCountersThread_k10;
            perfmon_readCountersThread = perfmon_readCountersThread_k10;
            perfmon_setupCounterThread = perfmon_setupCounterThread_k10;
            break;

        default:
            ERROR_MSG(Unsupported Processor);
            break;
    }


    for (i=0; i<perfmon_numThreads; i++) 
    {
        initThread(i,threads[i]);
    }
}

void
perfmon_finalize()
{
    int i;

    free(perfmon_threadData);

    for (i=0; i<perfmon_numThreads; i++)
    {
        free(perfmon_threadState[i]);
    }
    free(perfmon_threadState);

}
