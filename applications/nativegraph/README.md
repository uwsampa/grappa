Simple Graph Algorithms
-----------------------

This directory contains some graph algorithms implemented directly against Grappa's Graph data structure. These can be contrasted against the implementations in `applications/graphlab`, which are implemented at a higher level using the GraphLab API emulation.

Be warned, in some cases, for instance `bfs/bfs_beamer`, this "native" version is the fastest implementation, but in many cases, the GraphLab version is better optimized and more efficient, and this `simplegraph` version is more for demonstration purposes.
