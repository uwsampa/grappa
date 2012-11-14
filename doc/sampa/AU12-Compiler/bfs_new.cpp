#include <Grappa.hpp>

int *global bfs_tree;  // array of parent vertices

int *global frontier;  // array of vertex ids
int *global end;       // index of end of frontier

void bfs(int NV, int *global offsets, int *global edge_ends, int *global bfs_tree, int root) {
  int *global frontier = Grappa_malloc<int>(NV);

  // start with root as only thing in frontier
  frontier[0] = root;
  
  int start = 0
  int end = 1;

  // parent of root is self
  bfs_tree[root] = root;
  
  while (start != end) {
    int old_end = end;
    
    grappa_forall (int i = start; i < end; i++) {
      int v = frontier[i];
      grappa_forall (int j = offsets[v]; j < offsets[v+1]; j++) {
        int e = edge_ends[j];
        if (Grappa_delegate_cmp_swap(bfs_tree+e, -1, v)) {
          int index = Grappa_delegate_fetch_incr(end, 1);
          frontier[index] = e;
        }
      }
    }

    start = old_end;
    end = end;
  }
  
  Grappa_free(frontier);
  return t;
}


