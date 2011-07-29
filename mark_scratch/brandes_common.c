#include <stdio.h>
#include <stdlib.h>
#include "base.h"
#include "fastlist.h"
#include "brandes_common.h"

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
            printf("array[%lld] -> != %f is %f\n", i, v, array[i]);
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

int64 nodes = 0;
int64 cut_off = 0;
int64 edges_per_node;
int tasks_per_thread = 1;
int uctxs_per_task = 1;
uint64 start, finish;
uint64 long_fetch_time = 0;

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
    
    cut_off = 8;
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

int parse_command_line(int argc, char *argv[]) {
    int i;
    
    i = 1;
    while (i < argc) {
        if (argv[i][0] == '-') {
            if ((i + 1) >= argc)
                return 0;
            switch (argv[i][1]) {
                case    'e':
                case    'E':
                    edges_per_node = atoi(argv[i + 1]);
                    i += 2;
                    break;
                case    'n':
                case    'N':
                    nodes = atoi(argv[i + 1]);
                    i += 2;
                    break;
                case    't':
                case    'T':
                    tasks_per_thread = atoi(argv[i + 1]);
                    i += 2;
                    break;
                case    'c':
                case    'C':
                    cut_off = atoi(argv[i + 1]);
                    i += 2;
                    break;
                case    'd':
                case    'D':
                    long_fetch_time = atoi(argv[i + 1]);
                    i += 2;
                    break;
                case    'u':
                case    'U':
                    uctxs_per_task = atoi(argv[i + 1]);
                    i += 2;
                    break;                
                default:
                    return 0;
                }
            }
        else
            return 0;
        }
    if (cut_off == 0)
        cut_off = nodes;
    return 1;
}

void print_command_line_usage() {
    printf( "usage:\n"
            "    -e[dges] #\n"
            "    -n[odes] #\n"
            "    -t[asks] #\n"
            "    -c[utoff] #\n"
            "    -u[ctxs] #\n"
            "    -d[elay] #\n"
            );
}

int main(
    int argc,
    char *argv[]) {
    graph_node **gns;
    double *cb;
    
    if (!parse_command_line(argc, argv)) {
        print_command_line_usage();
        return 1;
        }

    if (nodes == 0) {
        printf("Running simple unit test graph\n");
        do_test();
        return 2;
        }
                
    gns = create_random_graph(nodes, edges_per_node);
    
    cb = compute_betweenness_centrality(gns, nodes);
    
    free(cb);
    printf("CB: %lld\n", (finish - start) / ONE_MILLION);
    
    return 0;
    }
    