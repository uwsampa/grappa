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

double *compute_betweenness_centrality(
    graph_node **gns,
    int64 nodes) {
    double *betweenness_centrality, *shortest_path_count, *dependency_value;
    int64 i, v, j, w, *vp;
    int64 *q, *s, *shortest_path_length, q_head, q_tail, s_index;
    int64 *change_set, change_set_index;
    int64 *touched_this_round;
    
    int enqueued;
    
    struct list **predecessor_lists;
    graph_node *gn;
    
    betweenness_centrality = malloc(sizeof(double) * nodes);
    shortest_path_count = malloc(sizeof(double) * nodes);
    dependency_value = malloc(sizeof(double) * nodes);
    
    q = malloc(sizeof(int64) * nodes);
    s = malloc(sizeof(int64) * nodes);
    shortest_path_length = malloc(sizeof(int64) * nodes);
    predecessor_lists = malloc(sizeof(struct list *) * nodes);
    change_set = malloc(sizeof(int64) * nodes);
    touched_this_round = malloc(sizeof(int64) * nodes);

    s_index = 0;
    
    for (i = 0; i < nodes; i++) {
        betweenness_centrality[i] = 0.0;
        shortest_path_count[i] = 0.0;
        dependency_value[i] = 0.0;
        predecessor_lists[i] = NULL;
        shortest_path_length[i] = -1;
        touched_this_round[i] = 0;
        }
        
    for (i = 0; i < nodes; i++) {
        s_index = 0;
        change_set_index = 0;
        
        shortest_path_count[i] = 1.0;
        shortest_path_length[i] = 0.0;
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
            
            if (!touched_this_round[v]) {
                change_set[change_set_index] = v;
                ++change_set_index;
                touched_this_round[v] = 1;
                }
            
            if (!predecessor_lists[v])
                predecessor_lists[v] = list_create(sizeof(int64));
            
            gn = gns[v];
            
            for (j = 0; j < gn->edge_count; j++) {
                w = gn->edges[j];
                enqueued = 0;
                if (shortest_path_length[w] < 0) {
                    // enqueue w to Q
                    q[q_head] = w;
                    ++q_head;
                    
                    shortest_path_length[w] = shortest_path_length[v] + 1;
                    enqueued = 1;
                    }
                if (shortest_path_length[w] == (shortest_path_length[v] + 1)) {
                    if (!enqueued) {
                        if (!touched_this_round[v]) {
                            change_set[change_set_index] = w;
                            ++change_set_index;
                            touched_this_round[v] = 1;
                            }
                        }
                    shortest_path_count[w] = shortest_path_count[w] + shortest_path_count[v];
                    if (!predecessor_lists[w])
                        predecessor_lists[w] = list_create(sizeof(int64));                    
                    list_append(predecessor_lists[w], &v);
                    }
                }
            }
    
        while (s_index != 0) {
            --s_index;
            w = s[s_index];
            assert(predecessor_lists[w] != NULL);
            list_iterator_init(predecessor_lists[w]);
            while ((vp = (int64 *)list_next(predecessor_lists[w])) != NULL) {
                v = *vp;
                dependency_value[v] = dependency_value[v] +
                    shortest_path_count[v] / shortest_path_count[w] *
                    (1.0 + dependency_value[w]);
                if (w != i)
                    betweenness_centrality[w] = betweenness_centrality[w] +
                                                dependency_value[w];
                }
            }

        for (j = 0; j < change_set_index; j++) {
            shortest_path_count[change_set[j]] = 0.0;
            dependency_value[change_set[j]] = 0.0;
            shortest_path_length[change_set[j]] = -1;
            assert(predecessor_lists[change_set[j]] != NULL);
            list_clear(predecessor_lists[change_set[j]]);
            touched_this_round[change_set[j]] = 0;
            }
            
        shortest_path_length[i] = -1;
        shortest_path_count[i] = 0.0;
        
        assert(is_all_same_double(shortest_path_count, 0.0, nodes) == 1);
        assert(is_all_same_double(dependency_value, 0.0, nodes) == 1);
        assert(is_all_same_int64(shortest_path_length, -1, nodes) == 1);
        }

    free(shortest_path_count);
    free(dependency_value);
    free(q);
    free(s);
    free(shortest_path_length);
    for (i = 0; i < nodes; i++) {
        if (predecessor_lists[i])
            list_destroy(predecessor_lists[i]);
        }
    free(predecessor_lists);
    
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
    
    