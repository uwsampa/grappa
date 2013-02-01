# Grappa
Enabling high-throughput graph processing on commodity clusters.

# Building
We currently use Make to build the components of Grappa into a static (or dynamic) library.

## Dependences
* Compiler
  * GCC >= 4.6.2 (not strict, but this is what we use/test with)
* External:
  * Boost
* Slightly modified versions distributed with Grappa:
  * GASNet (preferably compiled with the Infiniband conduit, but any conduit will do)
  * gflags
  * glog

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
The easiest way to run a Grappa program is to use the `mpi_run` build target that is included in each of the existing applications' directories.

The following example will build and run the sorting benchmark with 2 nodes, with 2 cores per node.

```bash
# in $GRAPPA_HOME/applications/sort/grappa
make -j mpi_run TARGET=sort.exe NNODE=2 PPN=2 GARGS='--aggregator_autoflush_ticks=2000000 -- --scale=10 --log2maxkey=64 --log2buckets=7'
```

# Writing Grappa programs
The programming interface for Grappa is highly in flux. We are adding features and redesigning interfaces often.

This guide will try to outline the key features of Grappa from an application's perspective and attempt to describe the intended uses for each, because many calls can only be made from certain situations (i.e. some calls marked with `ALLNODES` in the documentation must be called in a SIMD fashion on all nodes in order for any to continue).

## Setup in main()
Everything that goes in `main` is boiler-plate code. In a future version of the runtime/language this will probably be completely hidden from the programmer, but for now it is necessary for setup in all Grappa programs.

We fire up Grappa programs similarly to how MPI works. Each node must call `Grappa_init`, `Grappa_activate` to configure the system, set up the communication layers, global memory, etc. Then by calling `Grappa_run_user_main`, the first task in the system, "user_main", is created and started on Node 0. Finally, `Grappa_finish` will tear down the system.

## User main
Everything in the `user_main` function is run by the "main" task. There is actually nothing special about this task except that when it returns, the program terminates.

## Functionality overview

### Tasks
Tasks can be run on any node in the system. They have the ability to "suspend", waiting for a response or event to wake them. They can also "yield" if they know that they are not done with their work but want to give other tasks a chance to run.

There are two kinds of tasks: public tasks, which are made visible to other nodes and can be stolen, and private tasks which run only where they are spawned.

One of the benefits of our approach to multithreading is that within a Node, tasks and active messages are multiplexed onto a core so only one will ever be running at a given time, and context switches happen only at known places such as certain function calls, so no synchronization is ever needed to data local to your node. A fair rule of thumb is that anything that goes to another node may contain a context switch.

### Global memory
There are two kinds of Global addresses: Linear addresses, which refer to memory allocated in the global heap, and 2D addresses which generally refer to a chunk of memory on a particular node (usually things allocated on a task's stack or malloc'd on the local heap).

These addresses can be easily constructed from pointers with:

* `make_global`: creates a 2D address (works on anything)
* `make_linear`: creates a linear address (only valid if called with a pointer in the global heap)

Global allocator calls. For best results, probably should be called from in `user_main` only.

* `Grappa_malloc`, `Grappa_typed_malloc`: allocate global memory, will be allocated in a round-robin fashion across all nodes in the system.
* `Grappa_free`: free global memory allocated with the given global base address.

### Basic communication
* `Grappa_call_on`: The most basic communication primitive we have. This sends an active message to a particular node to execute, using the Aggregator to make larger messages. This means that it could take a significant amount of time for a message to come back, and they will come back in an arbitrary order.

### Delegates
Can be called from anywhere except in an active message (so anything given to `Grappa_call_on` or at the target side of a generic delegate operation). These delegate operations suspend the calling task until the result comes back, so sequential consistency can be assumed within a task.

* `Grappa_delegate_read_word`: Read a 64-bit integer from another node.
* `Grappa_delegate_write_word`: Write a 64-bit value on another node.
* `Grappa_delegate_fetch_and_add_word`: Atomic fetch and add, useful for synchronization.
* `Grappa_delegate_compare_and_swap_word`: Atomic compare and swap.
* Generic delegates which can run anything that doesn't involve suspending on another node.

### Feed-forward delegates
We found that in many of our applications, we don't need a result from some delegate operations, such as appending to a work list in a BFS traversal. In those cases, suspending a task to wait for the result is just tying up a stack for longer than necessary. Feed-forward delegates rely on the GlobalTaskJoiner to ensure that they all complete.

### Caches
Explicitly managed caches allow us to gather a larger chunk of global memory at a time, do computation on it, and release it back into global memory. So far only Incoherent caches have been implemented, so other synchronization is required. Note: to prevent storage from being heap-allocated for a cache object, pass a pointer to a buffer. Acquire happens implicitly on the first dereference or on a call to `block_until_acquired`, and will cause the task to suspend. Release happens in the destructor, or when `block_until_released` is called.

* `Incoherent<T>::RO`: only does work for `acquire` to gather data from global memory, useful for getting more than a single word (it pays off even when fetching 2 words to use a RO cache).
* `Incoherent<T>::WO`: only does `release`. Useful for distributing data into a global array.
* `Incoherent<T>::RW`: Does acquire and release.

### Loop constructs/fork-join parallelism.

* `fork_join_custom`: Primary method for running code on all nodes in the system. Launches a private task on each node to execute the given functor. Functions marked `ALLNODES`, such as `Grappa_barrier`'s should be called from in a `fork_join_custom` call.
* `async_parallel_for`: Recursive loop decomposition. Comes in several flavors, including:
	- `global_async_parallel_for`: call in SIMD context, splits up array among nodes (evenly-sized blocks of the array are done by each node), and does stealing to load balance, uses GlobalTaskJoiner to do a complete phase.
	- `async_parallel_for_private`: does recursive decomposition with all private tasks.
* `forall_local`: Has each node iterate over the part of a global array that is allocated local to them. On each node, a recursive decomposition is done, so it is alright to call arbitrary delegate operations, etc.

### Collectives/barriers
* `Grappa_allreduce`: called from SIMD context, arbitrary reduction operators can be specified. Can do an allreduce of an array, as well.
* `Grappa_barrier_suspending`: called from SIMD context, has calling task suspend until one task from each node has entered the barrier. Note: this is the most reliable kind of barrier because it does not interrupt the polling thread so that other tasks can still execute delegate operations on nodes that have entered the barrier.

## Example programs
Some suggestions for places to find examples of how to use these features:

* `system/AsyncParallelFor_Global_tests.cpp`
* `system/Cache_tests.cpp`
* `applications/sort/grappa/main.cpp` (`forall_local` in particular)
* `applications/suite/grappa/{centrality.cpp,main.cpp}`
* `system/Gups_tests.cpp`

# Debugging
First of all, Grappa is a very young system, so there are likely to be many bugs, and some functionality is particularly brittle at the moment. In the course of debugging our own programs, we have found ways to debug:

* GASNet has support for suspending applications to allow you to attach to them in gdb. In `system/Makefile` are a bunch of GASNet settings that make it freeze on an error, or you can send a signal to suspend all GASNet processes.
* `system/grappa_gdb.macros`: Some useful macros for introspection into grappa data structures. Also allows you to switch to a running task and see its stack. Add the macro to your `.gdbinit` and type `help grappa` in gdb to see commands and usage.
* Build with DEBUG mode on to get better stack traces.

