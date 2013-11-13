//Generates tuple graph representation for a few simple graphs

void meshgrid_graph(int64_t * num_edges, GlobalAddress<packed_edge> * tuple_edges, int n, int m);

void balanced_tree_graph(int64_t * num_edges, GlobalAddress<packed_edge> * tuple_edges, int lvs, int branches);

void complete_graph(int64_t * num_edges, GlobalAddress<packed_edge> * tuple_edges, int vertices);
