# Graph500 for Grappa

Another straightforward change.

Graph500 is a fairly new benchmark/competition aimed at encouraging better irregular application performance. Right now, it consists of a single benchmark: breadth-first traversal of a graph structure to create a BFS tree (really just an array `numVertices` long where each element holds the parent of that node). (The next specification will include a Single-Source Shortest Path kernel as well). [Official specification](http://www.graph500.org/specifications)


## Graph representations
The most commonly used internal graph representation and the one used throughout Graph500 is Compressed Sparse Row (CSR). In the Graph500 code, this is specified by each kind of implementation, so there is a different version for OMP, MPI, MTA, & Grappa. All of them take the general form of:

* xoff[2*NV+2]:	array of offsets specifying for each vertex which index (into xadj) to start and end on to find the edges associated with this vertex.
* xadj[NE]: array of vertex id's that represent the end vertex of each edge (start vertex comes from knowing which vertex id was used with xoff to index into xadj). CSR is used to do all of the graph computations.

The graph generators, on the other hand, typically create an unsorted edge list, which is just an array of start/end vertex ids.

## Compiling
Compiling for Serial/OpenMP/MTA is the same, copy the correct make.inc version out of `make-incs` and rename it to `make.inc`. Then you can call `make` from within `applications/graph500` and it will build it.

For Grappa, it's a bit more complicated (but similar to the workflow for the MPI version). 

1. `cd` into `applications/graph500/grappa`
2. Make sure the environment variable `GRAPPA_HOME` is set to the path to the root of your grappa clone (in bash: `export GRAPPA_HOME=$HOME/grappa`)
3. Build the executable: `make -j`, which jumps to $GRAPPA_HOME/system to build the Grappa library and then compiles and links the stuff in `grappa/`, creating `graph.exe`.

### Cleaning
Calling `make clean` will only clean out the generated files in `applications/graph500/grappa`. If you would need to compile with TAU_API=1 or DEBUG=1 or just need to make sure everything is up-to-date after a checkout, then you should call

	`make allclean`

Similar to how building works, in addition to cleaning locally, it will cd to your `$SOFXMT_HOME/system` directory and do a `make clean` there. This ensures that the next time you run `make` it will redo everything and you'll get a consistent executable.

## Running
Grappa applications can be run fairly automatically through the makefile using the `mpi_run` target. `applications/graph500/grappa/Makefile` includes `system/Makefile` which has a bunch of stuff for automatically generating include dependences, etc, and we get the `mpi_run` target from there as well.

Call the following command from within `applications/graph500/grappa`. This will actually recompile anything it needs to, then fire off a job through Slurm across multiple nodes. These parameters all have sane defaults, but they are all specified here to help demonstrate the available options.

	$ cd $GRAPPA_HOME/applications/graph500/grappa
	$ make mpi_run TARGET=graph.exe NNODE=4 PPN=8 GARGS='--aggregator_autoflush_ticks=2000000 --periodic_poll_ticks=5000 --num_starting_workers=1024 --v=0 -- -s 16 -e 16 -p'

To break that down a bit:

* TARGET: executable (also name of makefile target)
* NNODE: number of machines in cluster to use (not the same as Grappa's 'nodes' which more closely correspond to cores)
* PPN: processors per node (cores to use per machine), total number of Grappa nodes = NNODE*PPN
* GARGS: arguments to pass to grappa executable.
	- The first set correspond to flags used to tweak the Grappa runtime (specified in our code with GFlags). For more info about system flags, see documentation in `system/` or look directly in the code. These parameters are probably a good starting point as they have been used to get our best results so far. 
	- Flags after the standalone `--` are passed directly through to the Graph500 code. These include:
		* `-s`: scale (`numVertices = 2^scale`)
		* `-e`: edgefactor (`numEdges = numVertices*edgefactor`)
		* `-p`: checkpointing (read/write from checkpoint files to save time spent generating graphs). Checkpoint files should reside in `applications/graph500/grappa/ckpts`. Feel free to symlink to my directory of pre-made ckpts: `/sampa/home/bholt/grappa/applications/graph500/grappa/ckpts`.
		* `-n`: no verification step. this is dangerous to have on if you're making changes to the code because it makes it skip the expensive part where it makes sure generated BFS trees are valid with respect to the the original edgelist. However, it's useful if you're doing a parameter sweep or trying to get performance results for the BFS part itself.

Other options for compiling, etc:

* Compile with DEBUG=1 to get debugging symbols, less variables optimized out, etc.
