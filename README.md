# Grappa
Enabling high-throughput graph processing on commodity clusters.

**Don't read me, even though it says to in the name.**
TODO: update me.
For now, look in:
- New_loop_tests.cpp
- New_delegate_tests.cpp
- CompletionEvent_tests.cpp
- Collective_tests.cpp
- Cache_tests.cpp

# Building
We currently use Make to build the components of Grappa into a static (or dynamic) library.

## Dependences
* Compiler
  * GCC >= 4.7.2 (we not depend on C++11 features only present in 4.7.2 and newer)
* External:
  * Boost (preferably > v1.51)
* Slightly modified versions distributed with Grappa:
  * GASNet (preferably compiled with the Infiniband conduit, but any conduit will do)
  * gflags
  * glog
  * VampirTrace
* Running Slurm implementation

Build Grappa:

```bash
# build packaged dependences
cd $GRAPPA_HOME/tools
./build_deps.sh
# build Grappa library
cd $GRAPPA_HOME/system
make
```

## Documentation
The Grappa system directory is documented with Doxygen comments. To generate the documentation, you must first ensure you have doxygen installed, then:

```bash
cd $GRAPPA_HOME/system
doxygen
# for HTML, open: $GRAPPA_HOME/system/doxygen/html/index.html
```

# Running Grappa programs
## Using `grappa_srun.rb`
The Ruby script `bin/grappa_srun.rb` can be used to automatically manage the pesky `srun` flags and GASNET settings that are needed to run Grappa programs correctly on a number of machines that we've tested on.

### Running an application:
```bash
# in $GRAPPA_HOME/applications/graph500/grappa, with `graph.exe` built
../../../bin/grappa_srun.rb --nnode=2 --ppn=2 --freeze-on-error -- graph.exe --num_starting_workers=512 -- -s 10
```
### Running a Grappa test:
```bash
# in $GRAPPA_HOME/system
make -j New_delegate_tests.test
../bin/grappa_srun.rb --nnode=2 --ppn=2 --test=New_delegate_tests
```

## Using Makefile to run
Another way to run a Grappa program is to use the `mpi_run` build target that is included in each of the existing applications' directories. This does both the build and the `srun` run in one command.

The following example will build and run the sorting benchmark with 2 nodes, with 2 cores per node.

```bash
# in $GRAPPA_HOME/applications/sort/grappa
make -j mpi_run TARGET=sort.exe NNODE=2 PPN=2 GARGS='--aggregator_autoflush_ticks=2000000 -- --scale=10 --log2maxkey=64 --log2buckets=7'
```

# Writing Grappa programs
*Warning*: The programming interface for Grappa is highly in flux. We are adding features and redesigning interfaces often.

This guide will try to outline the key features of Grappa from an application's perspective at a high level. Detailed description of functionality is left out here, look at the generated Doxygen documentation for details. We have tried to put all of the useful Grappa features in "Modules" to make it easier to find them. Note that anything in the `impl` namespace is intended to be implementation that is not intended to be used directly in applications.

## Setup in main()
Everything that goes in `main` is boiler-plate code. In a future version of the runtime/language this will probably be completely hidden from the programmer, but for now it is necessary for setup in all Grappa programs:

```c
int main(int argc, char* argv[]) {
  Grappa_init(&argc, &argv);
  Grappa_activate();
  Grappa_run_user_main(&user_main, (void*)NULL);
  Grappa_finish(0);
}
```

We fire up Grappa programs similarly to how MPI works. Each node must call `Grappa_init`, `Grappa_activate` to configure the system, set up the communication layers, global memory, etc. Then by calling `Grappa_run_user_main`, the first task in the system, "user_main", is created and started on Node 0. Finally, `Grappa_finish` will tear down the system.

## User main
Everything in the `user_main` function is run by the "main" task. There is actually nothing special about this task except that when it returns, the program terminates.

```c
void user_main(void * ignore) {
  // do stuff
}
```

## System overview and terminology
### Cores, Locales, and Nodes, oh my!
A lot of colliding terminology here. Grappa runs a separate process per core, so the basic unit of locality is a Grappa `Core`. Within a core, there should not be any actual *parallelism*, so it shouldn't be necessary to worry about atomicity of operations except in the case of explicit context switches at communication and synchronization calls.

Physical "nodes" in a cluster have many cores. To avoid the overloaded "node" terminology, we refer to this group of Cores that share a single physical shared memory a `Locale`.

### Tasks and workers
Tasks can be run on any node in the system. They have the ability to "suspend", waiting for a response or event to wake them. They can also "yield" if they know that they are not done with their work but want to give other tasks a chance to run.

There are two kinds of tasks: public tasks, which are made visible to other nodes and can be stolen, and private tasks which run only where they are spawned.

One of the benefits of our approach to multithreading is that within a Core, tasks and active messages are multiplexed on a single core, so only one will ever be running at a given time, and context switches happen only at known places such as certain function calls, so no synchronization is ever needed to data local to your core. A fair rule of thumb is that anything that goes to another core may contain a context switch.

Tasks are just a functor. In order to execute them, they must be paired with a stack. This is done by Workers, which pull a task off the public or private task queue, execute it until completion (suspending while waiting on remote references or synchronization), and then going back to pull another task off a queue.

The task scheduler keeps track of all of the workers that are ready to execute or suspended at any given time.

### Memory
#### Global memory
There are two kinds of Global addresses: Linear addresses, which refer to memory allocated in the global heap, and 2D addresses which generally refer to a chunk of memory on a particular node (usually things allocated on a task's stack or malloc'd on the local heap).

These addresses can be easily constructed from pointers with:

* `make_global()`: creates a 2D address (works on anything)
* `make_linear()`: creates a linear address (only valid if called with a pointer in the global heap)

Global allocator calls. For best results, probably should be called from in `user_main` only.

* `Grappa_malloc()`, `Grappa_typed_malloc()`: allocate global memory, will be allocated in a round-robin fashion across all nodes in the system.
* `Grappa_free()`: free global memory allocated with the given global base address.

#### Locale shared memory
Memory can be allocated out of the heap of memory on a Locale in the cluster. Any memory that will be accessed by a message must be in locale shared memory in order to enable delivery of messages within a locale. Workers' stacks are implicitly in this locale shared memory. Functions to explicitly allocate and free from this shared heap:
* `Grappa::locale_alloc<T>(n)`
* `Grappa::locale_free(T*)`

### Communication
* `Grappa::message(Core c, F func)`: The most basic communication primitive we have. This sends an active message to a particular node to execute, using the Aggregator to make larger messages. This means that it could take a significant amount of time for a message to come back, and they will come back in an arbitrary order.
* Delegate operations are useful for reading and writing global memory, as well as doing arbitrary remote procedure calls. See "Delegates" module in Doxygen for these.
* Caches: useful for buffering data locally. See "Caches" module in Doxygen.
* Synchronization: see Doxygen.
* Loops: Instead of spawning tasks directly, it's almost always better to use a parallel loop of some sort. See Doxygen for details.

## Example programs
Some suggestions for places to find examples of how to use these features:

- `system/New_loop_tests.cpp`
- `system/New_delegate_tests.cpp`
- `system/CompletionEvent_tests.cpp`
- `system/Collective_tests.cpp`
- `system/Cache_tests.cpp`
- `applications/graph500/grappa/{bfs,bfs_local}.cpp`
- `applications/NPB/GRAPPA/IS/intsort.cpp`
- `applications/suite/grappa/{centrality.cpp,main.cpp}`
- `applications/pagerank/pagerank.cpp`

# Debugging
First of all, Grappa is a very young system, so there are likely to be many bugs, and some functionality is particularly brittle at the moment. In the course of debugging our own programs, we have found ways to debug:

* The Google logging library we use is *really* good at getting things in order and flushing correctly. Use them and trust them. Debugging verbosity can be changed per-file with `--vmodule`.
* GASNet has support for suspending applications to allow you to attach to them in gdb. In `system/Makefile` are a bunch of GASNet settings that make it freeze on an error, or you can send a signal to suspend all GASNet processes.
* `system/grappa_gdb.macros`: Some useful macros for introspection into grappa data structures. Also allows you to switch to a running task and see its stack. Add the macro to your `.gdbinit` and type `help grappa` in gdb to see commands and usage.
* Build with DEBUG mode on to get better stack traces.

## Performance debugging tips
* Grappa has a bunch of statistics that can be dumped (`Grappa::Statistics::merge_and_print()`), use these to find out basic coarse-grained information.
* Grappa also supports collecting traces of the statistics over time using VampirTrace. These can be visualized in Vampir. Build with `VAMPIR_TRACE=1` and `GOOGLE_SAMPLED=1`.
