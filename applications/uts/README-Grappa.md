# Copyright 2010-2012 University of Washington. All Rights Reserved.
# LICENSE_PLACEHOLDER
# This software was created with Government support under DE
# AC05-76RL01830 awarded by the United States Department of
# Energy. The Government has certain rights in the software.

Unbalanced Tree Search in-memory (UTS-mem). This is an extension of the UTS benchmark, where
we do the implicit traversal of UTS to build a tree in memory. The timed portion
is to traverse this tree. In this way, UTS-mem tests both dynamic load balancing and 
random access.

Usage:
# build Grappa library
cd $GRAPPA_HOME/system
make -j lib

# build uts
cd $GRAPPA_HOME/applications/uts
make -j uts_grappa.exe

# run Grappa uts-mem (on test tree)
make TARGET=uts_grappa.exe mpi_run



See README for the UTS documentation relating to tree generation. 
Sample trees are in sample_trees.{sh,csh}

UTS-mem uses
the same tree generation command line arguments. uts_grappa.exe has some
implementation specific arguments:
--vertices_size = <int>    Specify enough space for the tree (can use $SIZExx variables from sample_trees.sh)
--verify_tree   = <bool>   Verify the generated tree (default true)

Example usage:
The following command runs uts_grappa.exe on 8 cluster nodes, with 4 cores per node, on asmall tree, with reasonable runtime parameters.

source sample_trees.sh
make mpi_run TARGET=uts_grappa.exe PPN=4 NNODE=8 GARGS='--num_starting_workers=1024 --steal=1 --async_par_for_threshold=8 --chunk_size=100 --vertices_size=$(SIZE0) --aggregator_autoflush_ticks=2000000 --periodic_poll_ticks=10000 -- $(T1) -v 2'




Issues:

TODO:
Make the target for uts_grappa.exe to build Grappa lib
