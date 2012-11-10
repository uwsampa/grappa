#include <Grappa.hpp>

GlobalAddress<int> bfs_tree;  // array of parent vertices

GlobalAddress<int> frontier;  // array of vertex ids
GlobalAddress<int> end;       // index of end of frontier

void visit_neighbor(GlobalAddress<int> edge_ends, int j, int parent) {
  int e = Grappa_delegate_read(edge_ends+j);

  if (Grappa_delegate_cmp_swap(bfs_tree+e, -1, parent)) {
    int index = Grappa_delegate_fetch_incr(end, 1);
    Grappa_delegate_write(frontier+index, e);
  }
}

void visit_frontier(GlobalAddress<int> frontier, int i) {
  int v = Grappa_delegate_read(frontier+i);

  int buf[2];
  Incoherent<int>::RO cxoff(xoff+2*v, 2, buf);
  int vstart = cxoff[0];
  int   vend = cxoff[1];

  Grappa_forall<int,int,visit_neighbor>(xadj+vstart, vend-vstart, v);
}

void bfs(int NV, GlobalAddress<int> offsets, GlobalAddress<int> edge_ends, GlobalAddress<int> bfs_tree, int root) {
  GlobalAddress<int> frontier = Grappa_malloc<int>(NV);

  // start with root as only thing in frontier
  Grappa_delegate_write(frontier, root);
  
  int start = 0
  int end = 1;
  
  Grappa_delegate_write(bfs_tree+root, root); // parent of root is self
  
  while (start != end) {
    int old_end = end;
    
    Grappa_forall<int,visit_frontier>(frontier+start, end-start, );

    start = old_end;
    end = end;
  }
  
  Grappa_free(frontier);

  return t;
}

