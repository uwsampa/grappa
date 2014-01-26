Grappa
===============================================================================
Grappa is a runtime system for scaling irregular applications on commodity clusters. 

Grappa is a research project and is still young! Please expect things to break. Please do not expect amazing performance yet. Please ask for help if you run into problems. We're excited to make 

Requirements
-------------------------------------------------------------------------------
You must have the following to be able to build Grappa:

* Build system
  * Ruby >= 1.9.3
  * CMake >= 2.8.6
* Compiler
  * GCC >= 4.7.2 (we depend on C++11 features only present in 4.7.2 and newer)
  * Or: Clang >= 3.4
* External:
  * MPI (tested with OpenMPI >= 1.5.4 and Mvapich2 >= 1.7, but should work with any)

The configure script deals with some other dependences automatically. You may want to override the default behavior for your specific system. See [BUILD.md](BUILD.md) for more details.

In addition, our test and run scripts all assume your machine uses the Slurm job manager. You may still run jobs with using any other MPI launcher, but you'll have to set necessary environment variables yourself. See [doc/running.md](doc/running.md) for more details.

Quick Start
-------------------------------------------------------------------------------
Ensure you have the dependences described above. Then:

```bash
git clone git@github.com:uwsampa/grappa.git
cd grappa
./configure
cd build/Make+Release
make -j demo-hello_world.exe
```

Now you should have a binary which you can launch as an MPI job. If you have Slurm installed on your system, you can use our convenient job-launch script:
```bash
bin/grappa_srun.rb --nnode=2 --ppn=2 -- applications/demos/hello_world/demo-hello_world.exe
```

For more detailed instructions on building Grappa, see [BUILD.md](BUILD.md).

To run all our tests (a lengthy process) on a system using the Slurm job manager, do `make check-all-pass`. More information on testing is in [doc/testing.md](doc/testing.md).

Getting Help
-------------------------------------------------------------------------------
The best way to ask questions is to submit an issue on GitHub: by keeping questions there we can make sure the answers are easy for everyone to find. Submit an issue with the "Question" tag using this link: https://github.com/uwsampa/grappa/issues/new


# Writing Grappa programs #
*Warning*: The programming interface for Grappa is highly in flux. We are adding features and redesigning interfaces often.

This guide will try to outline the key features of Grappa from an application's perspective at a high level. Detailed description of functionality is left out here, look at the generated Doxygen documentation for details. We have tried to put all of the useful Grappa features in "Modules" to make it easier to find them. Note that anything in the `impl` namespace is intended to be implementation that is not intended to be used directly in applications.

As a general rule, the version of an API in the `Grappa` namespace is the most up-to-date. Anything with `Grappa_` is deprecated.

## Setup in main() ##
As with other systems like MPI, main has a couple standard setup calls that will be present in all Grappa programs.

```cpp
using namespace Grappa;
int main(int argc, char* argv[]) {
  init(&argc, &argv);
  // launches the single main task, started on core 0
  // when this task exits, the program ends 
  run([]{
    std::cout << "Hello world!\n";
  });
  finalize();
  return 0;
}
```

Output when compiled and run:

```bash
> bin/grappa_srun.rb --nnode=2 --ppn=2 -- ./hello_world.exe
# 0: Hello world! #
```

The call to `Grappa::init()` parses the command-line arguments (with `gflags`), sets up the communication layers, global memory, tasking, etc.

`Grappa::run()` launches the "main" Grappa task *on Core 0 only* (in contrast to the SPMD execution of MPI & UPC). This task represents a *global view* of the application, and will typically issue parallel loops or issue stealable tasks to spread work throughout the machine. When this task returns, the entire Grappa program terminates.

Finally, `Grappa::finalize()` tears down the system, communication layers, etc, terminating the program at the end. The final `return` call will not be reached.

## System overview and terminology ##
### Cores, Locales, and Nodes, oh my! ###
A lot of colliding terminology here. Grappa runs a separate process per core, so the basic unit of locality is a Grappa `Core`. Within a core, there should not be any actual *parallelism*, so it shouldn't be necessary to worry about atomicity of operations except in the case of explicit context switches at communication and synchronization calls.

Physical "nodes" in a cluster have many cores. To avoid the overloaded "node" terminology, we refer to this group of Cores that share a single physical shared memory a `Locale`.

### Tasks and workers ###
Tasks can be run on any node in the system. They have the ability to "suspend", waiting for a response or event to wake them. They can also "yield" if they know that they are not done with their work but want to give other tasks a chance to run.

Tasks are just a functor (a function with some state), and don't constitute an execution context on their own. Tasks get executed by Workers, which essentially just consist of a stack. Workers pull tasks off of a local task queue and execute a single one at a time on their stack. They execute the task to completion (suspending while waiting on remote calls or synchronization).

By default, tasks are "bound" to the core they are created on. That is, they will stay on their local task queue until they are eventually picked up by a worker on that core. Tasks can also be spawned "unbound", which puts them into a different queue. These tasks are load-balanced across all the cores in the cluster (using work-stealing). Therefore these "unbound" tasks have to handle being run from any core, and must be sure to fetch any additional data they need.

One of the benefits of our approach to multithreading is that within a Core, tasks and active messages are multiplexed on a single core, so only one will ever be running at a given time, and context switches happen only at known places such as certain function calls, so no synchronization is ever needed to data local to your core. A fair rule of thumb is that anything that goes to another core may contain a context switch.

#### Parallel Loops ####
Instead of spawning tasks individually, it's almost always better to use a parallel loop of some sort. These spawn loop iterations recursively until hitting a threshold. This prevents over-filling the task buffers for large loops, prevents excessive task overhead, and improves work-stealing by making it more likely that a single steal will generate significantly more work.

Basic loops include:
* `forall_global()`: launches public (stealable) tasks on all cores in the system.
* `forall()`: launches private tasks on all cores, regardless of locality.
* `forall(GlobalAddress,size_t,lambda)`: launches private tasks with iterations corresponding to a range of global address space to allow sequential access to array elements without excess communication.

See Doxygen for more details.

### Memory ###
#### Global memory ####
There are two kinds of Global addresses: Linear addresses, which refer to memory allocated in the global heap, and 2D addresses which generally refer to a chunk of memory on a particular node (usually things allocated on a task's stack or malloc'd on the local heap).

These addresses can be easily constructed from pointers with:

* `make_global()`: creates a 2D address (works on anything)
* `make_linear()`: creates a linear address (only valid if called with a pointer in the global heap)

Global allocator calls. For best results, probably should be called from in `user_main` only.

* `Grappa::global_alloc<T>()`: allocates global memory, will be allocated in a round-robin fashion across all nodes in the system.
* `Grappa::global_free()`: free global memory allocated with the given global base address.

#### Locale shared memory ####
Memory can be allocated out of the heap of memory on a Locale in the cluster. Any memory that will be accessed by a message must be in locale shared memory in order to enable delivery of messages within a locale. Workers' stacks are implicitly in this locale shared memory. Functions to explicitly allocate and free from this shared heap:
* `Grappa::locale_alloc<T>(n)`
* `Grappa::locale_free(T*)`

### Communication & Synchronization ###
* Delegate operations are essentially remote procedure calls. They can be blocking (must block if they return a value), or asynchronous. If asynchronous, they will have a synchronization object associated with them (typically a `GlobalCompletionEvent`) which can later be blocked on to ensure completion.
  * `Grappa::delegate::call(Core, lambda)`: the most basic unit of blocking delegate. Takes a destination Core and a lambda. The lambda will be executed atomically at the indicated core, and *must not attempt to block or yield*.
  * `Grappa::delegate::call<async,Sync>(Core, lambda)`: generic non-blocking delegate. Has a template parameter specifying the GlobalCompletionEvent it synchronizes with.
  * Many useful helpers exist to do simple delegate operations, such as `read`, `write`, `increment_async`, `write_async`, `fetch_and_add`, etc. Check them out in the Doxygen docs.
* Synchronization: see Doxygen.
* Caches: for manually buffering data locally. See "Caches" module in Doxygen.
* Collectives: primitive reduction support is available.
* `Grappa::message(Core c, F func)`: The most basic communication primitive we have. This sends an active message to a particular node to execute, using the Aggregator to make larger messages. This means that it could take a significant amount of time for a message to come back, and they will come back in an arbitrary order. **Messages should almost never need to be sent explicitly. It is almost always better to use a *delegate***.

## Example programs ##
Some suggestions for places to find *fairly up-to-date* examples of how to use these features:

- `system/New_loop_tests.cpp`
- `system/New_delegate_tests.cpp`
- `system/CompletionEvent_tests.cpp`
- `system/Collective_tests.cpp`
- `applications/graph500/grappa/{main_new,bfs_local_adj,cc_new}.cpp`
- `applications/NPB/GRAPPA/IS/intsort.cpp`
- `applications/suite/grappa/{centrality.cpp,main.cpp}`
- `applications/pagerank/pagerank.cpp`

