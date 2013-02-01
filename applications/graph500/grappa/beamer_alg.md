# Hybrid Top-down/bottom-up BFS algorithm
S. Beamer, K. Asanovic, D. Patterson. Direction-Optimizing Breadth-First Search. Supercomputing 2012.

```
def top_down_step(vertices, frontier, next, parents):
  for v in frontier:
    for n in neighbors[v]:
      if parents[n] == -1:
        parents[n] = v
        next << n

def bottom_up_step(vertices, frontier, next, parents):
  for v in vertices:
    if parents[v] == -1:
      for n in neighbors[v]:
        if n in frontier: # check 'depth' bits in bfs_tree?
          parents[v] = n
          next << v
          break
```

Transitions
- `n`: total number of vertices
- `n_f`: number of vertices in frontier
- `m_f`: number of edges to check from the frontier
- `m_u`: number of edges to check from unexplored vertices
- `alpha`: tuning parameter to make it transition to bottom-up a bit early
- `beta`: tuning parameter (transition back to top-down before end)
```
  top_down: (start)
    if (m_f > m_u/alpha) && growing  -> bottom_up
  
  bottom_up:
    if (n_f < n/beta) && shrinking   -> top_down
```

