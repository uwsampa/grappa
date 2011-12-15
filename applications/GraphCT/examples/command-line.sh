#!/bin/bash

# Run IO server
./luc_file_io/luc_srv >/dev/null 2>&1 &

# Run 1-betweenness centrality with 256 source vertices on
# data shared by CASS-MT's Task 14.
./GraphCT-CLI \
  -i ~rmfarber/sharedTask14/dimacs/2009-09.atlflood.graph \
  -t dimacs -o result.txt -z kcentrality 1 256

# Kill IO server
kill %1
