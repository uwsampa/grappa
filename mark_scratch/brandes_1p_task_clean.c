#include <stdio.h>
#include <stdlib.h>
#include "base.h"
#include "fastlist.h"
#include "brandes_common.h"
#include "tasklib.h"
  
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

struct il_data {
    int64   w;
    };

struct ol_data {
    search_data *sd;
    int64 *change_set, change_set_index;
    int64 *q, *s, q_head, q_tail, s_index;
    graph_node *gn;
    int64 i, v, j, w, *vp;
    uint64 start_long_fetch;
    };

double *compute_betweenness_centrality(
    graph_node **gns,
    int64 nodes) {
    int number_of_ol_tasks = tasks_per_thread;
    int number_of_il_tasks = uctxs_per_task;
    struct task    *ol_tasks, *il_tasks;
    struct ol_data *ol_data, *od;
    struct il_data *il_data;

    int     gi;
    double *betweenness_centrality;
    int64   *work_queue, work_queue_index, work_queue_size;


    ol_tasks = malloc(sizeof(struct task) * number_of_ol_tasks);
    ol_data = malloc(sizeof(struct ol_data) * number_of_ol_tasks);
    il_tasks = malloc(sizeof(struct task) * number_of_il_tasks);
    il_data = malloc(sizeof(struct il_data) * number_of_il_tasks);

    work_queue = malloc(sizeof(int64) * nodes);
    // add all work items to the task queue
    for(gi = 0; gi < cut_off; gi++)
        work_queue[gi] = gi;
    work_queue_index = 0;
    work_queue_size = cut_off;

    betweenness_centrality = malloc(sizeof(double) * nodes);
    for (gi = 0; gi < nodes; gi++)
        betweenness_centrality[gi] = 0.0;
     
    for(gi = 0; gi < number_of_ol_tasks; gi++) {
        od = &ol_data[gi];

        od->q = malloc(sizeof(int64) * nodes);
        od->s = malloc(sizeof(int64) * nodes);
        od->sd = malloc(sizeof(search_data) * nodes);
    
        od->change_set = malloc(sizeof(int64) * nodes);
        od->s_index = 0;
    
        for (od->i = 0; od->i < nodes; od->i++) {
            od->sd[od->i].shortest_path_count = 0.0;
            od->sd[od->i].dependency_value = 0.0;
            od->sd[od->i].predecessor_list = NULL;
            od->sd[od->i].shortest_path_length = -1;
            od->sd[od->i].touched_this_round = 0;
            }
        }   

    task_scheduler(ol_tasks, ol_data, number_of_ol_tasks, struct ol_data) {
		while (work_queue_index < work_queue_size) {
			od = cd;
			od->i = work_queue[work_queue_index];
			++work_queue_index;

			if (od->i == 5)
				start = rdtsc();
            
			od->s_index = 0;
			od->change_set_index = 0;
        
			od->sd[od->i].shortest_path_count = 1.0;
			od->sd[od->i].shortest_path_length = 0.0;
			od->q_head = od->q_tail = 0;
        
			// enqueue s to Q
			od->q[od->q_head] = od->i;
			++od->q_head;
			while (od->q_head != od->q_tail) {
				// dequeue v from Q
				od->v = od->q[od->q_tail];
				++od->q_tail;
            
				// push v to S
				od->s[od->s_index] = od->v;
				++od->s_index;
            
				if (!od->sd[od->v].touched_this_round) {
					od->change_set[cd->change_set_index] = od->v;
					++od->change_set_index;
					od->sd[od->v].touched_this_round = 1;
                }
            
				if (!od->sd[od->v].predecessor_list)
					od->sd[od->v].predecessor_list = list_create(sizeof(int64));
            
				od->gn = gns[od->v];

				prefetch(od->gn);
				od->start_long_fetch = rdtsc();
				yield();
				od = cd;
				while ((rdtsc() - od->start_long_fetch) < long_fetch_time)
					;
           
				od->j = 0;
				task_scheduler(il_tasks, il_data, number_of_il_tasks, struct il_data) { 
					while (od->j < od->gn->edge_count) {
						od->w = od->gn->edges[od->j];
						++od->j;

						prefetch(&od->sd[od->w]);
						cd->w = od->w;
						yield();
						od->w = cd->w;
                
						if (od->sd[od->w].shortest_path_length < 0) {
							// enqueue w to Q
							od->q[od->q_head] = od->w;
							++od->q_head;
                    
							od->sd[od->w].shortest_path_length =
								od->sd[od->v].shortest_path_length + 1;
						}
                
						if (od->sd[od->w].shortest_path_length ==
							(od->sd[od->v].shortest_path_length + 1)) {
							if (!od->sd[od->w].touched_this_round) {
								od->change_set[od->change_set_index] = od->w;
								++od->change_set_index;
								od->sd[od->w].touched_this_round = 1;
							}
							od->sd[od->w].shortest_path_count =
								od->sd[od->w].shortest_path_count +
								od->sd[od->v].shortest_path_count;
							if (!od->sd[od->w].predecessor_list)
								od->sd[od->w].predecessor_list = list_create(sizeof(int64));
							else {
								prefetch(od->sd[od->w].predecessor_list);
								cd->w = od->w;
								yield();
								od->w = cd->w;
							}
                                        
							list_append(od->sd[od->w].predecessor_list, &od->v);
						}
					}
				} end_scheduler();
			}

			task_scheduler(il_tasks, il_data, number_of_il_tasks, struct il_data) {
				while (od->s_index != 0) {
					--od->s_index;
					od->w = od->s[od->s_index];
					prefetch(&od->sd[od->w]);
					cd->w = od->w;
					yield();
					od->w = cd->w;
            
					assert(od->sd[od->w].predecessor_list != NULL);
            
					prefetch(od->sd[od->w].predecessor_list);
					cd->w = od->w;
					yield();
					od->w = cd->w;

            
					list_iterator_init(od->sd[od->w].predecessor_list);
					while ((od->vp = (int64 *)list_next(od->sd[od->w].predecessor_list)) !=
						   NULL) {
						od->v = *od->vp;
						od->sd[od->v].dependency_value = od->sd[od->v].dependency_value +
							od->sd[od->v].shortest_path_count /
							od->sd[od->w].shortest_path_count *
							(1.0 + od->sd[od->w].dependency_value);
						if (od->w != od->i)
							betweenness_centrality[od->w] = betweenness_centrality[od->w] +
								od->sd[od->w].dependency_value;
					}
				}
			} end_scheduler();
                
    
			od->j = 0;
			task_scheduler(il_tasks, il_data, number_of_il_tasks, struct il_data) {
				while(od->j < od->change_set_index) {
					od->w = od->change_set[od->j];
					od->j++;
					prefetch(&od->sd[od->w]);
            
					cd->w = od->w;
					yield();
					od->w = cd->w;
            
					od->sd[od->w].shortest_path_count = 0.0;
					od->sd[od->w].dependency_value = 0.0;
					od->sd[od->w].shortest_path_length = -1;
            
					prefetch(od->sd[od->w].predecessor_list);
					cd->w = od->w;
					yield();
					od->w = cd->w;
            
					assert(od->sd[od->w].predecessor_list != NULL);
					//list_clear(od->sd[od->w].predecessor_list);
					list_destroy(od->sd[od->w].predecessor_list);
					od->sd[od->w].predecessor_list = NULL;
					od->sd[od->w].touched_this_round = 0;
				}
			} end_scheduler();
            
			od->sd[od->i].shortest_path_length = -1;
			od->sd[od->i].shortest_path_count = 0.0;
        
			//assert(is_all_same_double(od->shortest_path_count, 0.0, nodes) == 1);
			//assert(is_all_same_double(od->dependency_value, 0.0, nodes) == 1);
			//assert(is_all_same_int64(od->shortest_path_length, -1, nodes) == 1);
		}

		finish = rdtsc();

		free(od->q);
		free(od->s);
		for (od->i = 0; od->i < nodes; od->i++) {
			if (od->sd[od->i].predecessor_list)
				list_destroy(od->sd[od->i].predecessor_list);
        }
		free(od->sd);
	} end_scheduler();

    free(ol_tasks); 
    free(ol_data);
    free(il_tasks);
    free(il_data);

    return betweenness_centrality;
    }
