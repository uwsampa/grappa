typedef struct _graph_node {
    int64 edge_count;
    int64 edges[0];
    } graph_node;

#define INVALID_EDGE -1

double *compute_betweenness_centrality(
    graph_node **gns,
    int64 nodes);
    
extern int64 nodes;
extern int64 cut_off;
extern int64 edges_per_node;
extern int tasks_per_thread;
extern int uctxs_per_task;
extern uint64 start, finish;
extern uint64 long_fetch_time;

int is_all_same_int64(int64 *array, int64 v, int64 count);
int is_all_same_double(double *array, double v, int64 count);
