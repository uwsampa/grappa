#include <stdio.h>
#include <stdlib.h>
#include "base.h"
#include "fastlist.h"
#include "brandes_common.h"
    
/*

Note: the following maps the author (Brandes) variable names to the ones used here:

   cb    : betweenness_centrality
   d     : shortest_path_length
   sigma : dependency_value
   omega : shortest_path_count 

 Note: this implementation was varified to be O(nm) by performing the following
 runs on a single processor:
 
    n       seconds	n	    m       nm
    2000	4.07    2000 	400000	800000000
    3000	12.64	3000 	900000	2700000000
    4000	28.66	4000 	1600000	6400000000
    5000	55.48	5000 	2500000	12500000000
    
*/

typedef struct _search_data {
        double shortest_path_count;
        double dependency_value;
        struct list *predecessor_list;
        int64 shortest_path_length;
        int64 touched_this_round;
        } search_data;

typedef struct _task_header {
    int restart_point;
    int task_has_started;
    int task_has_finished;
    } task_header;

typedef struct _uctx {
    // management
    int restart_point;
    int finished;
    // data
    int64   w;
    }   uctx;

typedef struct betweenness_centrality_task_data {
//    double *shortest_path_count, *dependency_value;
//    int64 *shortest_path_length;
//    struct list **predecessor_lists;
//    int64 *touched_this_round;
    search_data *sd;
    int64 *change_set, change_set_index;
    int64 *q, *s, q_head, q_tail, s_index;
    graph_node *gn;
    int64 i, v, j, w, *vp;
    uint64 start_long_fetch;
    int started_uctxs;
    int finished_uctxs;
    int uctx_number;
    int uctx_set;
    uctx    *uctx;
    } task_data;
    
typedef struct _task {
    task_header th;
    task_data   td;
    } task;

#define prefetch4(addr) \
    __asm__ volatile(   "prefetcht0 %0\n"   \
                        "prefetcht0 %1\n"   \
                        "prefetcht0 %2\n"   \
                        "prefetcht0 %3\n" : :   \
                         "m" (*(((uint8 *)addr)+0)), \
                         "m" (*(((uint8 *)addr)+64)), \
                         "m" (*(((uint8 *)addr)+128)), \
                         "m" (*(((uint8 *)addr)+192)) );

#define prefetch(addr) \
    __asm__ volatile(   "prefetcht0 %0\n"   \
                         : : "m" (*(((uint8 *)addr)+0)) );
                
#define yield(yp)    \
    th->restart_point = yp; \
    goto schedule_next_task;    \
    restart_point_##yp:

#define restart(yp) \
            case    yp:  goto    restart_point_##yp;

double *compute_betweenness_centrality(
    graph_node **gns,
    int64 nodes) {
    int number_of_tasks = tasks_per_thread;
    int number_of_uctxs = uctxs_per_task;
    task    *tasks;
    task_data   *td;
    task_header *th;
    int     gi;
    double *betweenness_centrality;
    int64   *work_queue, work_queue_index, work_queue_size;

    int current_task = 0;
    int started_tasks = 0;
    int finished_tasks = 0;
    
    tasks = malloc(sizeof(task) * number_of_tasks);
    
    for (gi = 0; gi < number_of_tasks; gi++) {
        tasks[gi].th.restart_point = -1;
        tasks[gi].th.task_has_finished = 0;
        tasks[gi].th.task_has_started = 0;
        }
        
    work_queue = malloc(sizeof(int64) * nodes);
    // add all work items to the task queue
    for(gi = 0; gi < cut_off; gi++)
        work_queue[gi] = gi;
    work_queue_index = 0;
    work_queue_size = cut_off;

    betweenness_centrality = malloc(sizeof(double) * nodes);
    for (gi = 0; gi < nodes; gi++) {
        betweenness_centrality[gi] = 0.0;
        }
     
    for(gi = 0; gi < number_of_tasks; gi++) {
        td = &tasks[gi].td;

        td->uctx = malloc(sizeof(uctx) * number_of_uctxs);
        td->q = malloc(sizeof(int64) * nodes);
        td->s = malloc(sizeof(int64) * nodes);
        td->sd = malloc(sizeof(search_data) * nodes);
    
        td->change_set = malloc(sizeof(int64) * nodes);
        td->s_index = 0;
    
        for (td->i = 0; td->i < nodes; td->i++) {
            td->sd[td->i].shortest_path_count = 0.0;
            td->sd[td->i].dependency_value = 0.0;
            td->sd[td->i].predecessor_list = NULL;
            td->sd[td->i].shortest_path_length = -1;
            td->sd[td->i].touched_this_round = 0;
            }
        }   

    start = rdtsc();
    
start_new_task:    
    th = &tasks[started_tasks].th;
    td = &tasks[started_tasks].td;
    th->task_has_started = 1;
    current_task = started_tasks;
    ++started_tasks;
    
    goto task_start;

task_end:
    th->task_has_finished = 1;
    ++finished_tasks;
    if (finished_tasks == started_tasks) {
        assert(work_queue_index == work_queue_size);
        goto end_function;
        }
    goto schedule_next_task;

schedule_next_task:
    if (started_tasks < number_of_tasks)
        goto start_new_task;
    
    // simple round robin scheduler.
    gi = 0;
    while (gi < number_of_tasks) {
        current_task = (current_task + 1);
        if (current_task >= number_of_tasks)
            current_task = 0;
        assert(tasks[current_task].th.task_has_started);
        if (!tasks[current_task].th.task_has_finished)
            break;
        ++gi;
        }
    assert(gi != number_of_tasks);
    th = &tasks[current_task].th;
    td = &tasks[current_task].td;

    switch (th->restart_point) {
        case    -1:
            assert(0 && "restart point is -1.  this is an error.");
            printf("Fatal error\n");
            exit(1);
        restart(1);
        //restart(2);
        //restart(3);
        //restart(4);
        
        default:
            assert(0 && "restart point not coded.\n");
            printf("Fatal error: unknown restart point: %d\n",
                th->restart_point);
            exit(2);
        }
    assert(0 && "corrupt restart point\n");
    printf("Fatal error\n");
    exit(1);

task_start:

    while (work_queue_index < work_queue_size) {
        td->i = work_queue[work_queue_index];
        ++work_queue_index;

        if (td->i == 5)
            start = rdtsc();
            
        td->s_index = 0;
        td->change_set_index = 0;
        
        td->sd[td->i].shortest_path_count = 1.0;
        td->sd[td->i].shortest_path_length = 0.0;
        td->q_head = td->q_tail = 0;
        
        // enqueue s to Q
        td->q[td->q_head] = td->i;
        ++td->q_head;
        while (td->q_head != td->q_tail) {
            // dequeue v from Q
            td->v = td->q[td->q_tail];
            ++td->q_tail;
            
            // push v to S
            td->s[td->s_index] = td->v;
            ++td->s_index;
            
            if (!td->sd[td->v].touched_this_round) {
                td->change_set[td->change_set_index] = td->v;
                ++td->change_set_index;
                td->sd[td->v].touched_this_round = 1;
                }
            
            if (!td->sd[td->v].predecessor_list)
                td->sd[td->v].predecessor_list = list_create(sizeof(int64));
            
            //prefetch(&gns[td->v]);
            //yield(2);
            //restart_point_3:
            
            td->gn = gns[td->v];

            prefetch(td->gn);
            td->start_long_fetch = rdtsc();
            yield(1);
            
            while ((rdtsc() - td->start_long_fetch) < long_fetch_time)
                ;
            
            //restart_point_1:

#define uctx_yield(n)   \
    td->uctx[td->uctx_number].restart_point = n;   \
    goto schedule_uctx; \
    restart_point_uctx_##n:

#define uctx_restart(yp) \
            case    yp:  goto    restart_point_uctx_##yp;

#define uctx_start_set(n)   \
            td->uctx_number = 0;    \
            td->uctx_set = n;   \
            td->started_uctxs = td->finished_uctxs = 0; \
            for (gi = 0; gi < number_of_uctxs; gi++)    \
                td->uctx[gi].finished = 0;

            uctx_start_set(1);
            td->j = 0;
            goto begin_uctx;
            
schedule_uctx:
            if (td->started_uctxs < number_of_uctxs)
                goto begin_uctx;
                
            gi = 0;
            while(gi < number_of_uctxs) {
                ++td->uctx_number;
                if (td->uctx_number >= number_of_uctxs)
                    td->uctx_number = 0;
                if (!td->uctx[td->uctx_number].finished)
                    break;
                ++gi;
                }
            assert(gi != number_of_uctxs);
            switch (td->uctx[td->uctx_number].restart_point) {
                uctx_restart(1);
                uctx_restart(11);
                uctx_restart(2);
                uctx_restart(21);
                uctx_restart(3);
                uctx_restart(31);
                default:
                    printf("panic in uctx, restart point = %d finished = %d ctx=%d\n",
                        td->uctx[td->uctx_number].restart_point,
                        td->uctx[td->uctx_number].finished,
                        td->uctx_number);
                    exit(1);
                    }
            printf("panic in ctx 2\n");
            exit(2);        
            
begin_uctx:
            td->uctx_number = td->started_uctxs;
            ++td->started_uctxs;

#define uctx_start(n)   \
    case    n:  goto uctx_set_##n;
            
            switch(td->uctx_set) {
                uctx_start(1);
                uctx_start(2);
                uctx_start(3);
                }

uctx_set_1:
            while (td->j < td->gn->edge_count) {
                    
                td->w = td->gn->edges[td->j];
                ++td->j;

                prefetch(&td->sd[td->w]);
                td->uctx[td->uctx_number].w = td->w;
                uctx_yield(1);
                td->w = td->uctx[td->uctx_number].w;
                
                if (td->sd[td->w].shortest_path_length < 0) {
                    // enqueue w to Q
                    td->q[td->q_head] = td->w;
                    ++td->q_head;
                    
                    td->sd[td->w].shortest_path_length = td->sd[td->v].shortest_path_length + 1;
                    }
                
                if (td->sd[td->w].shortest_path_length ==
                    (td->sd[td->v].shortest_path_length + 1)) {
                    if (!td->sd[td->w].touched_this_round) {
                        td->change_set[td->change_set_index] = td->w;
                        ++td->change_set_index;
                        td->sd[td->w].touched_this_round = 1;
                        }
                    td->sd[td->w].shortest_path_count = td->sd[td->w].shortest_path_count +
                        td->sd[td->v].shortest_path_count;
                    if (!td->sd[td->w].predecessor_list)
                        td->sd[td->w].predecessor_list = list_create(sizeof(int64));
                    else {
                        prefetch(td->sd[td->w].predecessor_list);
                        td->uctx[td->uctx_number].w = td->w;
                        uctx_yield(11);
                        td->w = td->uctx[td->uctx_number].w;
                        }
                                        
                    list_append(td->sd[td->w].predecessor_list, &td->v);
                    }
                }
#define uctx_end_set \
            td->uctx[td->uctx_number].finished = 1; \
            ++td->finished_uctxs;   \
            if (td->finished_uctxs < td->started_uctxs) \
                goto schedule_uctx;
            
            uctx_end_set;
            
            }
    
        uctx_start_set(2);
        goto begin_uctx;
uctx_set_2:
        while (td->s_index != 0) {
            --td->s_index;
            td->w = td->s[td->s_index];
            prefetch(&td->sd[td->w]);
            
            td->uctx[td->uctx_number].w = td->w;
            uctx_yield(2);
            td->w = td->uctx[td->uctx_number].w;
            
            assert(td->sd[td->w].predecessor_list != NULL);
            
            prefetch(td->sd[td->w].predecessor_list);
            td->uctx[td->uctx_number].w = td->w;
            uctx_yield(21);
            td->w = td->uctx[td->uctx_number].w;
            
            list_iterator_init(td->sd[td->w].predecessor_list);
            while ((td->vp = (int64 *)list_next(td->sd[td->w].predecessor_list)) != NULL) {
                td->v = *td->vp;
                td->sd[td->v].dependency_value = td->sd[td->v].dependency_value +
                    td->sd[td->v].shortest_path_count / td->sd[td->w].shortest_path_count *
                    (1.0 + td->sd[td->w].dependency_value);
                if (td->w != td->i)
                    betweenness_centrality[td->w] = betweenness_centrality[td->w] +
                                                td->sd[td->w].dependency_value;
                }
            }
        uctx_end_set;
                
    
        uctx_start_set(3);
        td->j = 0;
        goto begin_uctx;
uctx_set_3:
        while(td->j < td->change_set_index) {
            td->w = td->change_set[td->j];
            td->j++;
            prefetch(&td->sd[td->w]);
            
            td->uctx[td->uctx_number].w = td->w;
            uctx_yield(3);
            td->w = td->uctx[td->uctx_number].w;
            
            td->sd[td->w].shortest_path_count = 0.0;
            td->sd[td->w].dependency_value = 0.0;
            td->sd[td->w].shortest_path_length = -1;
            
            prefetch(td->sd[td->w].predecessor_list);
            td->uctx[td->uctx_number].w = td->w;
            uctx_yield(31);
            td->w = td->uctx[td->uctx_number].w;
            
            assert(td->sd[td->w].predecessor_list != NULL);
            //list_clear(td->sd[td->w].predecessor_list);
            list_destroy(td->sd[td->w].predecessor_list);
            td->sd[td->w].predecessor_list = NULL;
            td->sd[td->w].touched_this_round = 0;
            }
        uctx_end_set;
            
        td->sd[td->i].shortest_path_length = -1;
        td->sd[td->i].shortest_path_count = 0.0;
        
        //assert(is_all_same_double(td->shortest_path_count, 0.0, nodes) == 1);
        //assert(is_all_same_double(td->dependency_value, 0.0, nodes) == 1);
        //assert(is_all_same_int64(td->shortest_path_length, -1, nodes) == 1);
        }

    finish = rdtsc();

    free(td->q);
    free(td->s);
    for (td->i = 0; td->i < nodes; td->i++) {
        if (td->sd[td->i].predecessor_list)
            list_destroy(td->sd[td->i].predecessor_list);
        }
    free(td->sd);
    free(td->uctx);
    goto task_end;
    
end_function:
    
    return betweenness_centrality;
    }
