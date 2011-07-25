#include <stdio.h>
#include <stdlib.h>
#include "base.h"
#include "fastlist.h"

#define INVALID_EDGE -1

typedef struct _graph_node {
    int64 edge_count;
    int64 edges[0];
    } graph_node;

int64 rand64() {
    uint64 r1, r2;
    
    r1 = rand();
    r2 = rand();
    r1 = r1 << 32;
    r1 = r1 | r2;
    return (int64) r1;
    }

graph_node *create_node(int edges) {
    graph_node *gn;
    int64 i;
    
    gn = malloc(sizeof(*gn) + sizeof(int64) * edges);
    gn->edge_count = edges;
    for (i = 0; i < edges; i++)
        gn->edges[i] = INVALID_EDGE;
    
    return gn;
    }

graph_node **create_random_graph(
    int64 nodes,
    int64 max_edges_per_node) {
    
    graph_node **gns;
    int64 i, j;
    
    gns = malloc(sizeof(graph_node *) * nodes);
    for (i = 0; i < nodes; i++) {
        gns[i] = create_node(rand64() % max_edges_per_node);
        for (j = 0; j < gns[i]->edge_count; j++)
            gns[i]->edges[j] = rand() % nodes;
        }
    
    return gns;
    }

void destroy_graph(graph_node **gns, int64 nodes) {
    int64 i;
    
    for (i = 0; i < nodes; i++)
        free(gns[i]);
        
    free(gns);
    }
    
int is_all_same_double(double *array, double v, int64 count) {
    int64 i;
    
    i = 0;
    while (i < count) {
        if (array[i] != v) {
            printf("array[%lld] != %f is %f\n", i, v, array[i]);
            return 0;
            }
        ++i;
        }
    return 1;
    }

int is_all_same_int64(int64 *array, int64 v, int64 count) {
    int64 i;
    
    i = 0;
    while (i < count) {
        if (array[i] != v)
            return 0;
        ++i;
        }
    return 1;
    }
    
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

typedef struct _task_header {
    int restart_point;
    int task_has_started;
    int task_has_finished;
    } task_header;
    
typedef struct betweenness_centrality_task_data {
    double *shortest_path_count, *dependency_value;
    int64 *q, *s, *shortest_path_length, q_head, q_tail, s_index;
    int64 *change_set, change_set_index;
    int64 *touched_this_round;
    struct list **predecessor_lists;
    //graph_node *gn;
    //int64 i, v, j, w, *vp;
    graph_node *saved_gn;
    int64 saved_i, saved_w, saved_v, saved_j, *saved_vp;
    } task_data;
    
typedef struct _task {
    task_header th;
    task_data   td;
    } task;

#define prefetch0(addr) \
    __asm__ volatile(   "prefetcht0 %0\n"   \
                        "prefetcht0 %1\n"   \
                        "prefetcht0 %2\n"   \
                        "prefetcht1 %3\n" : :   \
                         "m" (*(((uint8 *)addr)+0)), \
                         "m" (*(((uint8 *)addr)+64)), \
                         "m" (*(((uint8 *)addr)+128)), \
                         "m" (*(((uint8 *)addr)+192)) );
    
    
#define prefetch_and_yield(addr, yp)    \
    prefetch0(addr);    \
    th->restart_point = yp; \
    goto schedule_next_task;    \
    restart_point_##yp:

#define save(x) td->saved_##x = ld.x
#define restore(x) ld.x = td->saved_##x

double *compute_betweenness_centrality(
    graph_node **gns,
    int64 nodes) {
    int number_of_tasks = 2;
    task    *tasks;
    task_data   *td;
    task_header *th;
    int     gi;
    double *betweenness_centrality;
    int64   *work_queue, work_queue_index, work_queue_size;
    struct {
        graph_node *gn;
        int64 i, v, j, w, *vp;
        } ld;
        
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
    for(gi = 0; gi < nodes; gi++)
        work_queue[gi] = gi;
    work_queue_index = 0;
    work_queue_size = nodes;

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
        current_task = (current_task + 1) % number_of_tasks;
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
        case    1:  goto    restart_point_1;
        }
    assert(0 && "corrupt restart point\n");
    printf("Fatal error\n");
    exit(1);
    
task_start:
    betweenness_centrality = malloc(sizeof(double) * nodes);
    td->shortest_path_count = malloc(sizeof(double) * nodes);
    td->dependency_value = malloc(sizeof(double) * nodes);

    td->q = malloc(sizeof(int64) * nodes);
    td->s = malloc(sizeof(int64) * nodes);
    td->shortest_path_length = malloc(sizeof(int64) * nodes);
    td->predecessor_lists = malloc(sizeof(struct list *) * nodes);
    td->change_set = malloc(sizeof(int64) * nodes);
    td->touched_this_round = malloc(sizeof(int64) * nodes);

    td->s_index = 0;
    
    for (gi = 0; gi < nodes; gi++) {
        betweenness_centrality[gi] = 0.0;
        }
        
    for (ld.i = 0; ld.i < nodes; ld.i++) {
        td->shortest_path_count[ld.i] = 0.0;
        td->dependency_value[ld.i] = 0.0;
        td->predecessor_lists[ld.i] = NULL;
        td->shortest_path_length[ld.i] = -1;
        td->touched_this_round[ld.i] = 0;
        }

    while (work_queue_index < work_queue_size) {
        ld.i = work_queue[work_queue_index];
        ++work_queue_index;

        td->s_index = 0;
        td->change_set_index = 0;
        
        td->shortest_path_count[ld.i] = 1.0;
        td->shortest_path_length[ld.i] = 0.0;
        td->q_head = td->q_tail = 0;
        
        // enqueue s to Q
        td->q[td->q_head] = ld.i;
        ++td->q_head;
        while (td->q_head != td->q_tail) {
            // dequeue v from Q
            ld.v = td->q[td->q_tail];
            ++td->q_tail;
            
            // push v to S
            td->s[td->s_index] = ld.v;
            ++td->s_index;
            
            if (!td->touched_this_round[ld.v]) {
                td->change_set[td->change_set_index] = ld.v;
                ++td->change_set_index;
                td->touched_this_round[ld.v] = 1;
                }
            
            if (!td->predecessor_lists[ld.v])
                td->predecessor_lists[ld.v] = list_create(sizeof(int64));
            
            save(v);
            save(i);
            prefetch_and_yield(gns[ld.v], 1);
            restore(v);
            restore(i);
            
            ld.gn = gns[ld.v];
            
            for (ld.j = 0; ld.j < ld.gn->edge_count; ld.j++) {
                ld.w = ld.gn->edges[ld.j];
                if (td->shortest_path_length[ld.w] < 0) {
                    // enqueue w to Q
                    td->q[td->q_head] = ld.w;
                    ++td->q_head;
                    
                    td->shortest_path_length[ld.w] = td->shortest_path_length[ld.v] + 1;
                    }
                if (td->shortest_path_length[ld.w] == (td->shortest_path_length[ld.v] + 1)) {
                    if (!td->touched_this_round[ld.v]) {
                        td->change_set[td->change_set_index] = ld.w;
                        ++td->change_set_index;
                        td->touched_this_round[ld.v] = 1;
                        }
                    td->shortest_path_count[ld.w] = td->shortest_path_count[ld.w] + td->shortest_path_count[ld.v];
                    if (!td->predecessor_lists[ld.w])
                        td->predecessor_lists[ld.w] = list_create(sizeof(int64));                    
                    list_append(td->predecessor_lists[ld.w], &ld.v);
                    }
                }
            }
    
        while (td->s_index != 0) {
            --td->s_index;
            ld.w = td->s[td->s_index];
            assert(td->predecessor_lists[ld.w] != NULL);
            list_iterator_init(td->predecessor_lists[ld.w]);
            while ((ld.vp = (int64 *)list_next(td->predecessor_lists[ld.w])) != NULL) {
                ld.v = *ld.vp;
                td->dependency_value[ld.v] = td->dependency_value[ld.v] +
                    td->shortest_path_count[ld.v] / td->shortest_path_count[ld.w] *
                    (1.0 + td->dependency_value[ld.w]);
                if (ld.w != ld.i)
                    betweenness_centrality[ld.w] = betweenness_centrality[ld.w] +
                                                td->dependency_value[ld.w];
                }
            }

        for (ld.j = 0; ld.j < td->change_set_index; ld.j++) {
            td->shortest_path_count[td->change_set[ld.j]] = 0.0;
            td->dependency_value[td->change_set[ld.j]] = 0.0;
            td->shortest_path_length[td->change_set[ld.j]] = -1;
            assert(predecessor_lists[td->change_set[ld.j]] != NULL);
            list_clear(td->predecessor_lists[td->change_set[ld.j]]);
            td->touched_this_round[td->change_set[ld.j]] = 0;
            }
            
        td->shortest_path_length[ld.i] = -1;
        td->shortest_path_count[ld.i] = 0.0;
        
        assert(is_all_same_double(td->shortest_path_count, 0.0, nodes) == 1);
        assert(is_all_same_double(td->dependency_value, 0.0, nodes) == 1);
        assert(is_all_same_int64(td->shortest_path_length, -1, nodes) == 1);
        }

    free(td->shortest_path_count);
    free(td->dependency_value);
    free(td->q);
    free(td->s);
    free(td->shortest_path_length);
    for (ld.i = 0; ld.i < nodes; ld.i++) {
        if (td->predecessor_lists[ld.i])
            list_destroy(td->predecessor_lists[ld.i]);
        }
    free(td->predecessor_lists);
    
    goto task_end;
    
end_function:
    
    return betweenness_centrality;
    }

/*
 This test is based on the graph and data from this page:
 
    http://www.sscnet.ucla.edu/soc/faculty/mcfarland/soc112/cent-ans.htm
    
 Although, the answers given there are incorrect (even for the problem
 as stated) and we modify it slightly adding a dummy zero node.  Adjustments
 made accordingly.

*/
void do_test() {
    graph_node  *nodes[8];
    double *cb;
    int i;
    
    nodes[0] = malloc(sizeof(graph_node));
    nodes[0]->edge_count = 0;
    nodes[1] = malloc(sizeof(graph_node) + 1 * sizeof(int64));
    nodes[1]->edge_count = 1;
    nodes[1]->edges[0] = 3;
    nodes[2] = malloc(sizeof(graph_node) + 1 * sizeof(int64));
    nodes[2]->edge_count = 1;
    nodes[2]->edges[0] = 3;
    nodes[3] = malloc(sizeof(graph_node) + 3 * sizeof(int64));
    nodes[3]->edge_count = 3;
    nodes[3]->edges[0] = 4;
    nodes[3]->edges[1] = 1;
    nodes[3]->edges[2] = 2;
    nodes[4] = malloc(sizeof(graph_node) + 2 * sizeof(int64));
    nodes[4]->edge_count = 2;
    nodes[4]->edges[0] = 5;
    nodes[4]->edges[1] = 3;
    nodes[5] = malloc(sizeof(graph_node) + 3 * sizeof(int64));
    nodes[5]->edge_count = 3;
    nodes[5]->edges[0] = 4;
    nodes[5]->edges[1] = 6;
    nodes[5]->edges[2] = 7;
    nodes[6] = malloc(sizeof(graph_node) + 2 * sizeof(int64));
    nodes[6]->edge_count = 2;
    nodes[6]->edges[0] = 5;
    nodes[6]->edges[1] = 7;
    nodes[7] = malloc(sizeof(graph_node) + 2 * sizeof(int64));
    nodes[7]->edge_count = 2;
    nodes[7]->edges[0] = 5;
    nodes[7]->edges[1] = 6;
    
    cb = compute_betweenness_centrality(nodes, 8);
    
    for (i = 0; i < 8; i++) {
        printf("node[%d] = %f\n", i, cb[i]);
        }
    assert(cb[0] == 0.0);
    assert(cb[1] == 0.0);
    assert(cb[2] == 0.0);
    assert(cb[3] == 18.0);
    assert(cb[4] == 18.0);
    assert(cb[5] == 16.0);
    assert(cb[6] == 0.0);
    assert(cb[7] == 0.0);
}

int main(
    int argc,
    char *argv[]) {
    graph_node **gns;
    double *cb;
    int64 nodes;
    
    if (argc != 3) {
        printf("usage: %s <nodes> <connectivity>\n", argv[0]);
        printf("Running simple unit test graph\n");
        do_test();
        return 1;
        }
    
    gns = create_random_graph(nodes = atoi(argv[1]), atoi(argv[2]));
    
    cb = compute_betweenness_centrality(gns, nodes);
    free(cb);
    return 0;
    }

void test(void *x) {
    prefetch0(*(unsigned long*)x);
    }
    
    