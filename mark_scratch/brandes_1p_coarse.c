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

void p(void *v) {
    prefetch(v);
    }
                        
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
            
            for (td->j = 0; td->j < td->gn->edge_count; td->j++) {
                td->w = td->gn->edges[td->j];
                //prefetch(&td->sd[td->w]);
                //yield(3);
                
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
                    list_append(td->sd[td->w].predecessor_list, &td->v);
                    }
                }
            }
    
        while (td->s_index != 0) {
            --td->s_index;
            td->w = td->s[td->s_index];
            //prefetch(&td->sd[td->w]);
            //yield(4);
            
            assert(td->sd[td->w].predecessor_lists != NULL);
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

        for (td->j = 0; td->j < td->change_set_index; td->j++) {
            //prefetch(&td->sd[td->change_set[td->j]]);
            //yield(5);
            
            td->sd[td->change_set[td->j]].shortest_path_count = 0.0;
            td->sd[td->change_set[td->j]].dependency_value = 0.0;
            td->sd[td->change_set[td->j]].shortest_path_length = -1;
            assert(predecessor_list[td->sd[td->change_set[td->j]] != NULL);
            //list_clear(td->sd[td->change_set[td->j]].predecessor_list);
            list_destroy(td->sd[td->change_set[td->j]].predecessor_list);
            td->sd[td->change_set[td->j]].predecessor_list = NULL;
            td->sd[td->change_set[td->j]].touched_this_round = 0;
            }
            
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
    
    goto task_end;
    
end_function:
    
    return betweenness_centrality;
    }
