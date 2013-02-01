void bfs(int NV, GlobalAddress<int> offsets, GlobalAddress<int> edge_ends, GlobalAddress<int> bfs_tree, int root) {
  GlobalAddress<int> frontier = Grappa_malloc<int>(NV);

  // start with root as only thing in frontier
  delegate_write(frontier, root);
  
  int start = 0
  int end = 1;
  
  delegate_write(bfs_tree+root, root); // parent of root is self
  
  while (start != end) {
    int old_end = end;
    
    Grappa_forall (int i = start; i < end; i++) {
      int v = delegate_read(frontier+i);

      ////////////////////
      int vstart = delegate_read(offsets+v);
      int vend   = delegate_read(offsets+v+1);
      ////////////////////
      int buf[2];
      Incoherent<int>::RO co(offsets+v, 2, buf);
      int vstart = co[0];
      int   vend = co[1];
      ////////////////////

      // for all neighbors
      Grappa_forall (int j = vstart; j < vend; j++) {
        int e = delegate_read(edge_ends+j);
        // try to become parent
        if (delegate_cmp_swap(bfs_tree+e, -1, v)) {
          // add child to next level
          int index = delegate_fetch_incr(end, 1);
          delegate_write(frontier+index, e);
        }
      }
    }

    start = old_end;
    end = end;
  }

  Grappa_free(frontier);
  return t;
}
