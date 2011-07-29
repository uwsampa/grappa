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

double *compute_betweenness_centrality(
    graph_node **gns,
    int64 nodes) {
    search_data *sd;
    double *betweenness_centrality;
    int64 i, v, j, w, *vp;
    int64 *q, *s, q_head, q_tail, s_index;
    int64 *change_set, change_set_index;
    graph_node *gn;
    
    
    betweenness_centrality = malloc(sizeof(double) * nodes);
    sd = malloc(sizeof(search_data) * nodes);
    
    q = malloc(sizeof(int64) * nodes);
    s = malloc(sizeof(int64) * nodes);
    change_set = malloc(sizeof(int64) * nodes);

    s_index = 0;
    
    for (i = 0; i < nodes; i++) {
        betweenness_centrality[i] = 0.0;
        sd[i].shortest_path_count = 0.0;
        sd[i].dependency_value = 0.0;
        sd[i].predecessor_list = NULL;
        sd[i].shortest_path_length = -1;
        sd[i].touched_this_round = 0;
        }
    
    start = rdtsc();
        
    for (i = 0; i < cut_off; i++) {
    
        if (i == 5)
            start = rdtsc();

        s_index = 0;
        change_set_index = 0;
        
        sd[i].shortest_path_count = 1.0;
        sd[i].shortest_path_length = 0.0;
        q_head = q_tail = 0;
        
        // enqueue s to Q
        q[q_head] = i;
        ++q_head;
        while (q_head != q_tail) {
            // dequeue v from Q
            v = q[q_tail];
            ++q_tail;
            
            // push v to S
            s[s_index] = v;
            ++s_index;
            
            if (!sd[v].touched_this_round) {
                change_set[change_set_index] = v;
                ++change_set_index;
                sd[v].touched_this_round = 1;
                }
            
            if (!sd[v].predecessor_list)
                sd[v].predecessor_list = list_create(sizeof(int64));
            
            gn = gns[v];
            
            for (j = 0; j < gn->edge_count; j++) {
                w = gn->edges[j];
                if (sd[w].shortest_path_length < 0) {
                    // enqueue w to Q
                    q[q_head] = w;
                    ++q_head;
                    
                    sd[w].shortest_path_length = sd[v].shortest_path_length + 1;
                    }
                if (sd[w].shortest_path_length == (sd[v].shortest_path_length + 1)) {
                    if (!sd[w].touched_this_round) {
                        change_set[change_set_index] = w;
                        ++change_set_index;
                        sd[w].touched_this_round = 1;
                        }
                    sd[w].shortest_path_count = sd[w].shortest_path_count + sd[v].shortest_path_count;
                    if (!sd[w].predecessor_list)
                        sd[w].predecessor_list = list_create(sizeof(int64));                    
                    list_append(sd[w].predecessor_list, &v);
                    }
                }
            }
    
        while (s_index != 0) {
            --s_index;
            w = s[s_index];
            assert(sd[w].predecessor_list != NULL);
            list_iterator_init(sd[w].predecessor_list);
            while ((vp = (int64 *)list_next(sd[w].predecessor_list)) != NULL) {
                v = *vp;
                sd[v].dependency_value = sd[v].dependency_value +
                    sd[v].shortest_path_count / sd[w].shortest_path_count *
                    (1.0 + sd[w].dependency_value);
                if (w != i)
                    betweenness_centrality[w] = betweenness_centrality[w] +
                                                sd[w].dependency_value;
                }
            }

        for (j = 0; j < change_set_index; j++) {
            sd[change_set[j]].shortest_path_count = 0.0;
            sd[change_set[j]].dependency_value = 0.0;
            sd[change_set[j]].shortest_path_length = -1;
            assert(sd[change_set[j]].predecessor_list != NULL);
            //list_clear(sd[change_set[j]].predecessor_list);
            list_destroy(sd[change_set[j]].predecessor_list);
            sd[change_set[j]].predecessor_list = NULL;
            sd[change_set[j]].touched_this_round = 0;
            }
            
        sd[i].shortest_path_length = -1;
        sd[i].shortest_path_count = 0.0;
        
        //assert(is_all_same_double(shortest_path_count, 0.0, nodes) == 1);
        //assert(is_all_same_double(dependency_value, 0.0, nodes) == 1);
        //assert(is_all_same_int64(shortest_path_length, -1, nodes) == 1);
        }

    finish = rdtsc();
    
    free(q);
    free(s);
    for (i = 0; i < nodes; i++) {
        if (sd[i].predecessor_list)
            list_destroy(sd[i].predecessor_list);
        }
    free(sd);
    
    return betweenness_centrality;
    }
