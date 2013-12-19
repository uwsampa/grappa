# Grappa
Enabling high-throughput graph processing on commodity clusters.

## Quick Start
Ensure that you have CMake version >= 2.8.6, GCC version >= 4.7, and MPI on your path. If you have Boost version >= 1.51 installed, find where it is. To use the convenient job-launch script we provide, you must have Slurm on your system as well. Then:

```bash
git clone git@github.com:uwsampa/grappa.git
cd grappa
./configure --cc=$(which gcc) --cxx=$(which cxx) --boost=/path/to/boost/install
cd build/Make+Release
make -j demo-hello_world.exe
bin/grappa_srun.rb --nnode=2 --ppn=2 -- applications/demos/hello_world/demo-hello_world.exe
```

## Building
We use CMake to build Grappa, including downloading and building some dependencies, and the set of applications that we provide. CMake can generate scripts for a number of different build systems, but Makefiles are the best-tested for Grappa.

See more detailed building instructions in [BUILD.md](BUILD.md).

## Dependences
* Compiler
  * GCC >= 4.7.2 (we depend on C++11 features only present in 4.7.2 and newer)
* External:
  * Boost ( > v1.51)
* Slightly modified versions distributed with Grappa:
  * GASNet (preferably compiled with the Infiniband conduit, but any conduit will do)
  * glog
* Downloaded and built automatically:
  * gflags
  * gperftools (with `GOOGLE_PROFILER ON` or `TRACING` defined)
  * VampirTrace (with `TRACING` defined)
* Slurm job manager (in theory just need to be able to launch MPI jobs, but Slurm is the best tested)

## Documentation
The Grappa system directory is documented with Doxygen comments. To generate the documentation, you must verify that you have Doxygen installed, then build the `docs` target. For example:

```bash
# from build/Make+Release
make docs
```

This will generate doxygen HTML and PDF documentation in: `build/doxygen`. So you can open in your browser: `<grappa_dir>/build/doxygen/html/index.html`.

# Running Grappa programs
## Using `grappa_srun.rb`
The Ruby script `bin/grappa_srun.rb` can be used to automatically manage the pesky `srun` flags and GASNET settings that are needed to run Grappa programs correctly on a number of machines that we've tested on. Try running `bin/grappa_srun.rb --help` for more detailed usage information.

### Running an application:
```bash
# in build/Make+Release:
# build the desired executable
make -j graph_new.exe
# launch a Slurm job to run it:
bin/grappa_srun.rb --nnode=2 --ppn=2 -- graph_new.exe --scale=26 --bench=bfs --nbfs=8 --num_starting_workers=512
```

### Running a Grappa test:
```bash
# in your configured directory, (e.g. build/Make+Release)
# (builds and runs the test)
make -j check-New_delegate_tests
# or in two steps:
make -j New_delegate_tests.test
bin/grappa_srun.rb --nnode=2 --ppn=2 --test=New_delegate_tests
```

# Writing Grappa programs
*Warning*: The programming interface for Grappa is highly in flux. We are adding features and redesigning interfaces often.

This guide will try to outline the key features of Grappa from an application's perspective at a high level. Detailed description of functionality is left out here, look at the generated Doxygen documentation for details. We have tried to put all of the useful Grappa features in "Modules" to make it easier to find them. Note that anything in the `impl` namespace is intended to be implementation that is not intended to be used directly in applications.

As a general rule, the version of an API in the `Grappa` namespace is the most up-to-date. Anything with `Grappa_` is deprecated.

## Setup in main()
As with other systems like MPI, main has a couple standard setup calls that will be present in all Grappa programs.

```cpp
using namespace Grappa;
int main(int argc, char* argv[]) {
  init(&argc, &argv);
  // launches the single main task, started on core 0
  // when this task exits, the program ends 
  run([]{
    
  });
  finalize();
  return 0;
}
```

The call to `Grappa::init()` parses the command-line arguments (with `gflags`), sets up the communication layers, global memory, tasking, etc.

`Grappa::run()` launches the "main" Grappa task *on Core 0 only* (in contrast to the SPMD execution of MPI & UPC). This task represents a *global view* of the application, and will typically issue parallel loops or issue stealable tasks to spread work throughout the machine. When this task returns, the entire Grappa program terminates.

Finally, `Grappa::finalize()` tears down the system, communication layers, etc, terminating the program at the end. The final `return` call will not be reached.

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

#### Parallel Loops
Instead of spawning tasks individually, it's almost always better to use a parallel loop of some sort. These spawn loop iterations recursively until hitting a threshold. This prevents over-filling the task buffers for large loops, prevents excessive task overhead, and improves work-stealing by making it more likely that a single steal will generate significantly more work.

Basic loops include:
* `forall_global()`: launches public (stealable) tasks on all cores in the system.
* `forall_global_private()`: launches private tasks on all cores, regardless of locality.
* `forall(GlobalAddress,size_t,lambda)`: launches private tasks with iterations corresponding to a range of global address space to allow sequential access to array elements without excess communication.

See Doxygen for more details.

### Memory
#### Global memory
There are two kinds of Global addresses: Linear addresses, which refer to memory allocated in the global heap, and 2D addresses which generally refer to a chunk of memory on a particular node (usually things allocated on a task's stack or malloc'd on the local heap).

These addresses can be easily constructed from pointers with:

* `make_global()`: creates a 2D address (works on anything)
* `make_linear()`: creates a linear address (only valid if called with a pointer in the global heap)

Global allocator calls. For best results, probably should be called from in `user_main` only.

* `Grappa::global_alloc<T>()`: allocates global memory, will be allocated in a round-robin fashion across all nodes in the system.
* `Grappa::global_free()`: free global memory allocated with the given global base address.

#### Locale shared memory
Memory can be allocated out of the heap of memory on a Locale in the cluster. Any memory that will be accessed by a message must be in locale shared memory in order to enable delivery of messages within a locale. Workers' stacks are implicitly in this locale shared memory. Functions to explicitly allocate and free from this shared heap:
* `Grappa::locale_alloc<T>(n)`
* `Grappa::locale_free(T*)`

### Communication & Synchronization
* Delegate operations are essentially remote procedure calls. They can be blocking (must block if they return a value), or asynchronous. If asynchronous, they will have a synchronization object associated with them (typically a `GlobalCompletionEvent`) which can later be blocked on to ensure completion.
  * `Grappa::delegate::call(Core, lambda)`: the most basic unit of blocking delegate. Takes a destination Core and a lambda. The lambda will be executed atomically at the indicated core, and *must not attempt to block or yield*.
  * `Grappa::delegate::call_async<Sync>(Core, lambda)`: generic non-blocking delegate. Has a template parameter specifying the GlobalCompletionEvent it synchronizes with.
  * Many useful helpers exist to do simple delegate operations, such as `read`, `write`, `increment_async`, `write_async`, `fetch_and_add`, etc. Check them out in the Doxygen docs.
* Synchronization: see Doxygen.
* Caches: for manually buffering data locally. See "Caches" module in Doxygen.
* Collectives: primitive reduction support is available.
* `Grappa::message(Core c, F func)`: The most basic communication primitive we have. This sends an active message to a particular node to execute, using the Aggregator to make larger messages. This means that it could take a significant amount of time for a message to come back, and they will come back in an arbitrary order. **Messages should almost never need to be sent explicitly. It is almost always better to use a *delegate***.

## Example programs
Some suggestions for places to find *fairly up-to-date* examples of how to use these features:

- `system/New_loop_tests.cpp`
- `system/New_delegate_tests.cpp`
- `system/CompletionEvent_tests.cpp`
- `system/Collective_tests.cpp`
- `applications/graph500/grappa/{main_new,bfs_local_adj,cc_new}.cpp`
- `applications/NPB/GRAPPA/IS/intsort.cpp`
- `applications/suite/grappa/{centrality.cpp,main.cpp}`
- `applications/pagerank/pagerank.cpp`

# Debugging
First of all, Grappa is a very young system, so there are likely to be many bugs, and some functionality is particularly brittle at the moment. In the course of debugging our own programs, we have found ways to debug:

* Build with `./configure --mode=Debug` to get better stack traces (note, you'll have to use a different CMake-generated build directory, but this prevents confusing situations where not all files were built for debug).
* The Google logging library we use is *really* good at getting things in order and flushing correctly. Use them and trust them. Debugging verbosity can be changed per-file with `--vmodule`. See [their documentation](http://google-glog.googlecode.com/svn/trunk/doc/glog.html).
* GASNet has support for suspending applications to allow you to attach to them in gdb. Calling `grappa_srun.rb` with `--freeze-on-error` will enable this feature.
* **(TO BE FIXED SOON)** `system/grappa_gdb.macros`: Some useful macros for introspection into grappa data structures. Also allows you to switch to a running task and see its stack. Add the macro to your `.gdbinit` and type `help grappa` in gdb to see commands and usage.

## Performance debugging tips
* Grappa has a bunch of statistics that can be dumped (`Grappa::Statistics::merge_and_print()`), use these to find out basic coarse-grained information. You can also easily add your own using `GRAPPA_DEFINE_STAT()`.
* Grappa also supports collecting traces of the statistics over time using VampirTrace. These can be visualized in Vampir. **(SUPPORT in CMake coming soon)** Build with `VAMPIR_TRACE=1` and `GOOGLE_SAMPLED=1`.

## Tracing
Grappa supports sampled tracing via VampirTrace. These traces can be visualized using the commercial tool, [Vampir](http://www.vampir.eu/). When tracing, what happens is all of the metrics specified with `GRAPPA_DEFINE_STAT()` are sampled at a regular interval using sampling interrupts by Google's `gperftools` library, and saved to a compressed trace using the VampirTrace open source library. This results in a trace with the individual values of all of the Grappa statistics on all cores, over the execution.

Before configuring, ensure you have VampirTrace built somewhere. If VampirTrace is not built anywhere, there is a script to download and build it in `tools/`. Make sure to specify where to install it with the `--prefix` flag:

```bash
> ./tools/vampirtrace.rb --prefix=/sampa/share/grappa-third-party
```

To use tracing, a separate Grappa build configuration must be made using either the '--tracing' or '--vampir' flags. If Vampir was installed in your `third-party` directory, just use '--tracing'; otherwise, use '--vampir' to specify the path to a separate VampirTrace install. For example:

```bash
> ./configure --mode=Release --vampir=/opt/vampirtrace --third-party=/opt/grappa-third-party
```

Tracing configurations have `+Tracing` in their name. Cd into the build directory and build and run as usual. After running a Grappa executable, you should see trace output files show up in your build directory:

* `${exe}.otf`: main file that links other trace files together
* `${exe}.*.events.z`: zipped trace events per process

For now, tracing must be explicitly enabled for a particular region of execution, using `Statistics::start_tracing()` and `Statistics::stop_tracing()`. For example:

```cpp
int main(int argc, char* argv[]) {
  init(&argc, &argv);
  run([]{
    
    // setup, allocate global memory, etc
    
    Statistics::start_tracing();
    
    // performance-critical region
    
    Statistics::stop_tracing();
  });
  finalize();
}
```

