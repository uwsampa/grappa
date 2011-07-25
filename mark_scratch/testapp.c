#include "base.h"

typedef struct _thread_context {
    uint16  context_size;
    } thread_context;

/*
 graphs are stored in memory as a binary blob of compacted graph nodes.
 As in:
 graph_node|graph_edge|graph_edge|...|graph_node|graph_edge|...|graph_node|..

*/

#define MAX_EDGES_PER_NODE          8
#define MAX_NODES_PER_SYSTEM_NODE   4096

typedef struct _graph_edge {
    uint64  weight;
    uint64  dest_node;
    }   graph_edge;

typedef struct _graph_node {
    uint64  node_id;
    uint16  number_of_outgoing_edges;
    graph_edge  edges[MAX_EDGES_PER_NODE];
    }   graph_node;

typedef struct _remote_node_fetch_request {
    uint32  system_node;
    uint64  node_id;
    } remote_node_fetch_request;
    
typedef struct _result_matrix_element {
    uint64  weight;
    } result_matrix_element;

typedef struct _result_matrix_column {
    result_matrix_element   results[0];
    } result_matrix_column;
    
typedef struct _result_matrix_part {
    result_matrix_column    columns;
    } result_matrix_part;

    
typedef struct _node_data {
    uint32 system_node_id;
    
    uint64 local_graph_node_base;
        
    graph_node *local_graph_nodes;
        
    uint64 local_graph_size;
    
    uint64 total_number_of_graph_nodes;
    
    uint64 local_result_matrix_columns;
    
    uint64  total_number_of_system_nodes;
    uint64  *graph_nodes_at_each_system_node;
    
    local_ptr result_matrix_base;
    
    } node_data;
    
inline graph_node *local_node_id_to_graph_node(node_data *nd, uint64 node_id) {
    local_ptr lp;

    assert(node_id >= nd->local_graph_node_base);
    assert(node_id < (nd->local_graph_node_base + nd->local_graph_nodes));
    
    return &nd->local_graph_nodes[node_id - nd->local_graph_node_base];
    }
    
inline remote_node_fetch_request remote_node_id_to_remote_node_fetch_request(
    node_data *nd,
    uint64 node_id) {
    remote_node_fetch_request rdfr;
    
    assert(node_id < nd->local_graph_node_base ||
            node_id >= (nd->local_graph_node_base | nd->local_graph_nodes));
    assert(node_id < total_number_of_graph_nodes);

    rdfr.system_node = node_id / MAX_NODES_PER_SYSTEM_NODE;
    rdfr.node_id = node_id;
    
    return rdfr;
    }



