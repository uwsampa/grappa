# Grappa Tutorial
## Section 1: Hello World
Topics in this section:

- global-view programming model
- what's that in main? (init/run/finalize)
- `on_all_cores()`

Here is the simplest possible Grappa program:

```cpp
///////////////////////////////
// tutorial/hello_world_1.cpp
///////////////////////////////
#include <Grappa.hpp>
#include <iostream>
int main(int argc, char *argv[]) {
  Grappa::init(&argc, &argv);
  Grappa::run([]{
    std::cout << "Hello world from the root task!\n";
  });
  Grappa::finalize();
}
```

When run with 4 cores (2 nodes, with 2 processes per node) using the Slurm launcher, we get the following output:

```bash
> make tutorial-hello_world_1
> bin/grappa_srun.rb --nnode=2 --ppn=2 -- tutorial/hello_world_1.exe
Hello world from the root task!
```

Grappa jobs, like MPI & UPC, are launched with a separate process on each node in the job (known as SPMD -- Single Program Multiple Data). In this case, when Slurm launches the 2-node job, it executes `hello_world_1.exe` on both nodes. However, Grappa is primarily designed to be a "global view" model, which means that rather than coordinating where all parallel SPMD processes are at and how they divide up the data, the programmer is encouraged to think of the system as a large single shared memory, use data structures that intelligently distribute themselves across the cores, and write parallel loops that get executed in parallel across the whole machine. To that end, the body of the `Grappa::run()` call spawns the "root" task on a single core. This task determines the lifetime of the program: when it returns, the Grappa program ends. The intention is that this task will spawn many more tasks and synchronize with all of them and only return when all of the work has been completed. Because it only runs once, the output is only printed once.

Breaking down the rest of what goes on in main:

- `init()`: parses command-line flags and environment variables, and then initializes all of Grappa's subsystems, such as the communication and threading systems, and global heap allocation.
- `run()`: starts the scheduling (tasking and communication) loop, and on Core 0 only spawns the main Grappa task. All cores stay inside this function until this main task returns, which immediately signals the others to exit the scheduling loop.
- `finalize()`: cleans up the process, closing communication channels and freeing memory; does not return.

### SPMD Variant
To further illustrate the global-view vs. SPMD models, we can add a bit of SPMD execution back into our Grappa program:

```cpp
///////////////////////////////
// tutorial/hello_world_2.cpp
///////////////////////////////
#include <Grappa.hpp>
#include <Collective.hpp>
#include <iostream>
int main(int argc, char *argv[]) {
  Grappa::init(&argc, &argv);
  Grappa::run([]{
    std::cout << "Hello world from the root task!\n";
    Grappa::on_all_cores([]{
      std::cout << "Hello world from Core " << Grappa::mycore() << " of " << Grappa::cores()
                << " (locale " << Grappa::mylocale() << ")"<< "\n";
    });
    std::cout << "Exiting root task.\n";
  });
  Grappa::finalize();
}
```

This time when run on 4 cores, we may get output like this:

```bash
> make tutorial-hello_world_2
> bin/grappa_srun.rb --nnode=2 --ppn=2 -- tutorial/hello_world_2.exe
Hello world from the root task!
Hello world from Core 1 of 4 (locale 0)
Hello world from Core 0 of 4 (locale 0)
Hello world from Core 3 of 4 (locale 1)
Hello world from Core 2 of 4 (locale 1)
Exiting root task.
```

This should look familiar to the MPI programmer. The call to `on_all_cores()` spawns a task on each core and then suspends the calling task (the root task in this case) until all those tasks have completed. Therefore, we do not see the "Exiting" message until after all of the "Hello"s, though those may arrive in any order. 

This also introduces the API for identifying cores. In Grappa, each core is a separate destination. The physical node on which a core resides is referred to as its "locale" (borrowed from Chapel's terminology). The next section will refer to these functions more when dealing with how memory is partitioned among cores.

## Section 2: Global memory
In Grappa, all memory on all cores is addressable by any other core. This is done using the `GlobalAddress<T>` template class. In the spirit of other Partitioned Global Address Space (PGAS) languages and runtimes, Grappa provides mechanisms for easily distributing data across the various memories of nodes in a cluster.

This section will:

- Describe global addressing, partitioning and the global heap
- Introduce the various ways of addressing and allocating global memory

### Global Addresses & Allocators
Global addresses fall into one of three different categories:

- **2D addresses**: can address any memory on a core, including memory on stacks of currently-executing tasks, or memory allocated using `malloc` or `new`
- **Linear addresses**: cyclicly partitioned among cores, allocated from the global heap using `global_alloc<T>()`, freed with `global_free()`
- **Symmetric addresses**: same address valid on all cores, allocated from the global heap using `symmetric_global_alloc<T>()`, freed with `global_free()`
  
Pretty much anywhere in the Grappa API that takes a GlobalAddress will work with any of these types (they are all the same class), but they will have different behavior in terms of how they map to cores.

#### 2D Addresses

The first, 2D addresses, are the simplest. They represent just the pairing of a pointer, and the Core where that pointer is valid. These can be created using the call `make_global()`, and will typically reference either data on a worker's execution stack, or something allocated from the core-private heap (using `new`). Consider the following example (note: the rest of `main()` has been left out, see the full source at `tutorial/addressing.cpp`):

```cpp
long count = 0;
GlobalAddress<long> g_count = make_global( &count );
LOG(INFO) << "global address -- valid on core:" << g_count.core() << ", pointer:" << g_count.pointer();
```

The call to `make_global()` creates a new global address with the given pointer to the `count` variable on this task's stack and encodes the core where it is valid (`Grappa::mycore()`) with it.

#### Linear addresses
A portion of each node's global memory is set aside specifically for the *global heap*. This amount is determined by the command-line flag `--global_heap_fraction` (which defaults to 0.5). Allocations from this heap are distributed in a block-cyclic round-robin fashion across cores. Each core has a region of memory set aside for the global heap, with a heap offset pointer specific for the core. To allocate from this heap, the following functions are used:

- `GlobalAddress<T> Grappa::global_alloc<T>(size_t num_elements)`: allocates space for `num_elements` of size `T`, starting on some arbitrary core
- `void Grappa::global_free(GlobalAddress<T>)`: frees global heap allocation

*Linear* global addresses, returned by the global heap allocation function, are specifically designed for working with allocations from the global heap. Arithmetic on linear addresses will resolve to the correct core given the heap's block size, and on that core, will use the core's heap offset pointer to give the correct pointer to data.

We will illustrate this with an example. Here we allocate an array of 48 elements, which, due to `block_size` being set to 64 bytes, allocates 8 to each core before moving on to the next core.

```cpp
auto array = global_alloc<long>(48);
for (auto i=0; i<48; i++) {
  std::cout << "[" << i << ": core " << (array+i).core() << "] ";
}
std::cout << "\n";
```

```bash
> grappa_srun.rb --nnode=2 --ppn=2 -- tutorial/addressing_linear.exe
[0: core 0] [1: core 0] [2: core 0] [3: core 0] [4: core 0] [5: core 0] [6: core 0] [7: core 0]
[8: core 1] [9: core 1] [10: core 1] [11: core 1] [12: core 1] [13: core 1] [14: core 1] [15: core 1]
[16: core 2] [17: core 2] [18: core 2] [19: core 2] [20: core 2] [21: core 2] [22: core 2] [23: core 2]
[24: core 3] [25: core 3] [26: core 3] [27: core 3] [28: core 3] [29: core 3] [30: core 3] [31: core 3]
[32: core 0] [33: core 0] [34: core 0] [35: core 0] [36: core 0] [37: core 0] [38: core 0] [39: core 0]
[40: core 1] [41: core 1] [42: core 1] [43: core 1] [44: core 1] [45: core 1] [46: core 1] [47: core 1]
```

#### Symmetric addresses
Another kind of allocation that is possible is a "symmetric" allocation. This allocates a copy of the struct on each core from the global heap, returning a GlobalAddress that has is valid (and has the same offset, hence "symmetric") on every core. Symmetric global addresses are typically for data structures where it is desired to have something to refer to no matter which core one is on. *Due to limitations right now, you must pad the struct to be a multiple of the block size. This can be done using the macro: `GRAPPA_BLOCK_ALIGNED`*.

Below is an example of allocating a struct on all cores:

```cpp
/////////////////////////////////////
// tutorial/addressing_symmetric.cpp
/////////////////////////////////////
#include <Grappa.hpp>
#include <Collective.hpp>
#include <GlobalAllocator.hpp>

using namespace Grappa;

struct Data {
  size_t N;
  long *buffer;
  
  void init(size_t N) {
    this->N = N;
    this->buffer = new long[32];
  }
} GRAPPA_BLOCK_ALIGNED;

int main(int argc, char *argv[]) {
  init(&argc, &argv);
  run([]{    
    // allocate a copy of Data on every core out of the global heap
    GlobalAddress<Data> d = symmetric_global_alloc< Data >();
    
    on_all_cores([d]{
      // use `->` overload to get pointer to local copy to call the method on
      d->init(1024);
    });
    
    // now we have a local copy of the struct available anywhere
    on_all_cores([d]{
      d->buffer[0] = d->N;
    });
  });
  finalize();
}
```

In this example, we want a copy of the Data struct on each core so tasks can access a local version no matter where they run. After calling `symmetric_global_alloc()` the structs have not been initialized, so we must call `init()` on each copy. Here we use the `->` operator overload for symmetric addresses to get the pointer to the local copy and call the method on it. Finally, we can now reference that allocated struct by just passing around the GlobalAddress to the symmetric allocation.

## Section 3: Delegate operations
One of the core tenets of Grappa's programming model is the idea of *delegating* remote memory accesses to the core that owns the memory. By ensuring that only one core ever accesses each piece of memory, we can provide strong atomicity guarantees to Grappa programs that even single-node multithreaded applications typically do not. It also allows us to more confidently play with the memory hierarchy to optimize for low-locality access by careful management of caches and pre-fetching. Finally, delegating work to where the data is provides several advantages: it allows work to be distributed evenly across the machine and in some cases minimizes communication, provided the data is laid out well.

<!-- (leaving out details about cooperative scheduling) For one thing, this plays well with applications with low locality, as it allows work to be distributed evenly across the machine, provided the data is distributed well. It also plays a key role in providing a simple consistency model to programmers: due to Grappa's cooperative scheduling of tasks and communication within a core, atomicity is guaranteed between yield points (i.e. communication). -->

If you want data that resides on another core, you will likely use a delegate operation of some sort to accomplish your work. Delegates send a request to another core and block the caller; eventually, the other core executes the request and sends a response which wakes the caller and may return a value. By blocking the caller and executing the delegate atomically, Grappa provides sequentially consistent access to distributed memory (more about this in some of our papers).

The most basic unit of delegates is `call()`, which has a couple different forms for convenience:

```cpp
T call(Core dest, []() -> T { /* (on 'dest') */ })
U call(GlobalAddress<T> gp, [](T *p) -> U { /* (on gp.core(), p = gp.pointer()) */ })
```

The provided lambda is executed on the specified core, blocking the caller. The return value of the lambda is returned to the caller when it is woken. The only restriction on code in the delegate is that is *must not suspend or yield to the scheduler*. This is to ensure the communication workers always make progress. An assertion will fail if this is violated. To accomplish blocking remotely, a delegate may spawn a task to do blocking work, (or for delegates that specifically need to block on a remote synchronization object, see another alternative form: `call(Core, Mutex, []{})`.

Some simple helper delegates for common operations are provided; they are implemented using the above generic delegates.

- `T read(GlobalAddress<T>)`: reads memory at that address
- `void write(GlobalAddress<T>, T val)`: writes a value into memory at that address
- `T fetch_and_add(GlobalAddress<T>, T inc)`: increments the value in memory and returns the previous value
- `bool compare_and_swap(GlobalAddress<T>, T cmp, T val)`: if value is equal to `cmp`, sets it to `val` and returns `true`, else returns `false`

Delegate operations have some template parameters that allow their behavior to be customized, such as making them "asynchronous". This will be covered in the "Intermediate" part of the tutorial.

The following example demonstrates using delegates to access memory in the global heap, distributed among all the cores.

```cpp
size_t N = 50;
GlobalAddress<long> array = global_alloc<long>(N);

// simple global write
for (size_t i = 0; i < N; i++) {
  delegate::write( array+i, i );
}

for (size_t i = 0; i < N; i += 10) {
  // boring remote read
  long value = delegate::read( array+i );
  std::cout << "[" << i << "] = " << value;
  
  // do some arbitrary computation
  double v = delegate::call(array+i, [](long *a){ return tan(*a); });
  std::cout << ", tan = " << v << std::endl;
}
```


```bash
> make tutorial-delegates && bin/grappa_srun.rb --nnode=2 --ppn=2 -- tutorial/delegates.exe
0: [0] = 0, tan = 0
0: [10] = 10, tan = 0.648361
0: [20] = 20, tan = 2.23716
0: [30] = 30, tan = -6.40533
0: [40] = 40, tan = -1.11721
```

This is still using the single root task to do all the work, so it is all still serial. The next section will cover how to spawn lots of parallel work efficiently.

## Tasking
**Be aware:** Terminology about threading is very overloaded; everyone means something different when talking about them. In Grappa, we try to avoid using the term "thread". Instead, we have *tasks* which are a (typically small) unit of work and *workers* which execute tasks. This is explained in more detail in this section.

The most basic unit of parallelism in Grappa is the *task*. A *task* is a unit of work that has some state and some code to run; this is typically specified using a C++11 *lambda*, but can also be specified with a more traditional C++ *functor* (a class with `operator()` overloaded). Tasks are not run immediately after they are created; instead, they go on a queue of un-started tasks.

*Workers* are lightweight user-level threads that are *cooperatively scheduled* (sometimes also known as *fibers*), which means that they run uninterrupted on a core until they choose to *yield* or *suspend*. A worker takes an un-started task off the queue, executes it to completion, suspending or yielding as the task specifies. When the task finishes (returns from the lambda/functor), the worker goes off to find another task to execute.

One of the benefits of our approach to multithreading is that within a Core, tasks and active messages are multiplexed on a single core, so only one will ever be running at a given time, and context switches happen only at known places such as certain function calls, so no synchronization is ever needed to data local to your core. A fair rule of thumb is that anything that goes to another core may contain a context switch.

### Bound/unbound tasks

By default, tasks are "bound" to the core they are created on. That is, they will stay on their local task queue until they are eventually picked up by a worker on that core. Tasks can also be spawned "unbound", which puts them into a different queue. These tasks are load-balanced across all the cores in the cluster (using work-stealing). Therefore these "unbound" tasks have to handle being run from any core, and must be sure to fetch any additional data they need.

### Spawning tasks

The `spawn` command creates a new task and automatically adds it to the queue of tasks which Workers pull from:

```cpp
// 'bound' task, will run on the same core
spawn([]{ /* task code */ });

// 'unbound' task, may be load-balanced to any core
spawn<unbound>([]{ /* task code */ });
```

### Task synchronization
Spawns happen "asynchronously". That is, the task that called "spawn" continues right away, not waiting for the spawned task to get run. To ensure that a task has been executed before performing some operation, it must be synchronized with explicitly. 

Grappa provides a number of different options for synchronization, which we will cover more fully in a later section. Tasks may use any of the synchronization primitives directly, but here we will demonstrate just one way: using a `CompletionEvent`. Remember that the Grappa program terminates when the "run" task completes, so if that task does not block on spawned tasks completing, then they may not execute.

```cpp
run([]{
  LOG(INFO) << "'main' task started";
  
  CompletionEvent joiner;
  
  spawn(&joiner, []{
    LOG(INFO) << "task ran on " << mycore();
  });
  
  spawn<unbound>(&joiner, []{
    LOG(INFO) << "task ran on " << mycore();
  });
  
  joiner.wait();
  
  LOG(INFO) << "all tasks completed, exiting...";
});
```

Possible output:

```
0: 'main' task started
1: task ran on 1
0: task ran on 0
0: all tasks completed, exiting
```

When the *main* task spawned the two other tasks, it passed in a pointer to the CompletionEvent it made. This caused "spawn" to enroll the tasks with the CompletionEvent, and causes them to automatically signal their "completion" when they finish. This allows the main task, after enrolling the two spawned tasks, to suspend, until the last enrolled task (could be either of them) signals completion, which then wakes the main task, and when the main task completes, this signals the Grappa program to come to an end.

### Parallel loops
Instead of spawning tasks individually, it's almost always better to use a parallel loop of some sort. In addition to just looking better, these parallel loops also go to a significant amount of effort to run efficiently. For instance, they spawn loop iterations recursively until hitting a threshold. This prevents over-filling the task buffers for large loops, prevents excessive task overhead, and improves work-stealing by making it more likely that a single steal will generate significantly more work. They also spread out spawns across cores in the system, and when iterating over a region of linear global addresses, schedule the iterations to be as close to their data as possible.

The basic parallel loop is `forall()`, but it can be invoked in a couple different ways. First, the iteration range can be specified in a couple different ways:

- `forall(startIndex, nIterations, [](int64_t i){ })`: Specify a start index and a number of iterations. The iterations will be split evenly across all the cores, broken up into evenly-sized blocks.
- `forall(address, nElements, [](T&){ })`: Specify a linear address (start of an allocation from the global heap, for instance), and a number of elements. Iterations will be executed *at the core where the corresponding element lives*, and the lambda will be passed a simple reference to the element.
- `forall_here(startIndex, nIterations, [](int64_t i){ })`: Like the above forall, except instead of spreading iterations across all cores, it spawns them all locally (though if spawning unbound tasks, they may be moved).

Each `forall` loop accepts a couple different forms of lambda, allowing for a bit more control. For instance, a `forall` over elements in an array of `double`s, for instance, could be invoked:

```cpp
// just a reference to the element
forall(array, 100, [](double& e){ /* ... */ });

// reference + the absolute index of the element
forall(array, 100, [](int64_t i, double& e){ /* ... */ });

// a range of 'n' iterations, starting with 'start', and a pointer to the first one.
// (not often used outside of library implementations)
forall(array, 100, [](int64_t start, int64_t n, double * e){ /* ... */ });
```

Finally, each of the parallel loops allows specifying template parameters which affect how they run. The most notable of which are:
- TaskMode (Bound/Unbound): Just as `spawn` can optionally spawn "unbound" tasks which are load-balanced, parallel loops take the same parameter and use it when spawning tasks.
- SyncMode (Blocking/Async): By default, loops block the caller until complete, but one can specify "async" to make the call non-blocking. Most parallel loops use a GlobalCompletionEvent for synchronization. To ensure all "async" loops have run, a task can call `wait` on the GlobalCompletionEvent used for the loop (specified by another template parameter). By default, all `forall`s use the same GCE, so best practice is to use `async` inside of an outer blocking `forall` (or a call to `finish()`), which will ensure all the inner asyncs have finished before continuing.
- Threshold: This specifies how many iterations are run serially by a single task. Parallel loops recursively spawn tasks to split the iterations evenly, and stop recursing only when reaching this Threshold. By default, loops use the command-line flag `--parallel_loop_threshold`, but a different threshold can be specified statically if needed (for instance, if you want to ensure that each iteration gets its own task, you would specify Threshold=1).
- GlobalCompletionEvent: this parameter statically specifies a GCE to use for synchronizing the iterations of the loop. Change this only if you cannot use the default GCE for some reason (e.g. you are already using the default GCE concurrently in another context).

Because of a bunch of overloaded declarations, these template parameters can be specified in roughly any order, with the rest being left as default. So each of these is valid: `forall<unbound>`, `forall<async>`, `forall<async,unbound>`, `forall<unbound,async,1>`, `forall<&my_gce>`, ...

## Bringing it all together: GUPS

We will use a simple benchmark to illustrate the use of parallel loops and delegate operations. "GUPS", which stands for "Giga-Updates Per Second" is a measure and a benchmark for random access rate. The basic premise is to see how quickly you can issue updates to a global array, but the updates are indexed by another array.

```cpp
// applications/demos/gups/gups.cpp
// make -j TARGET=gups4.exe mpi_run GARGS=" --sizeB=$(( 1 << 28 )) --loop_threshold=1024" PPN=8 NNODE=12

#include <Grappa.hpp>

using namespace Grappa;

// define command-line flags (third-party 'gflags' library)
DEFINE_int64( sizeA, 1 << 30, "Size of array that GUPS increments" );
DEFINE_int64( sizeB, 1 << 20, "Number of iterations" );

// define custom statistics which are logged by the runtime
// (here we're not using these features, just printing them ourselves)
GRAPPA_DEFINE_STAT( SimpleStatistic<double>, gups_runtime, 0.0 );
GRAPPA_DEFINE_STAT( SimpleStatistic<double>, gups_throughput, 0.0 );

int main(int argc, char * argv[]) {
  init( &argc, &argv );
  run([]{

    // allocate target array from the global heap
    auto A = global_alloc<int64_t>(FLAGS_sizeA);
    // fill the array with all 0's (happens in parallel on all cores)
    Grappa::memset( A, 0, FLAGS_sizeA );
    
    // allocate another array
    auto B = global_alloc<int64_t>(FLAGS_sizeB);
    // initialize the array with random indices 
    forall( B, FLAGS_sizeB, [](int64_t& b) {
      b = random() % FLAGS_sizeA;
    });

    double start = walltime();

    // GUPS algorithm:
    // for (int i = 0; i < sizeB; i++) {
    //   A[B[i]] += 1;
    // }

    gups_runtime = walltime() - start;
    gups_throughput = FLAGS_sizeB / gups_runtime;

    LOG(INFO) << gups_throughput.value() << " UPS in " << gups_runtime.value() << " seconds";
    
    global_free(B);
    global_free(A);
    
  });
  finalize();
}
```

Here we've setup the GUPS benchmark, but left out the main loop that does the updates. We first initialized Grappa, then `run` the main task. This task will allocate two arrays from the global heap: A, which is the array we'll update, and B, which we fill with random numbers which will index into the A array. Next, we will step through a couple different implementations of the main GUPS loop to demonstrate the features we've discussed so far.

### Simple `forall`
```cpp
// applications/demos/gups/gups1.cpp
forall(0, FLAGS_sizeB, [=](int64_t i){
  delegate::fetch_and_add( A + delegate::read(B+i), 1);
});
```
This implementation uses the form of `forall` that takes a start index and a number of iterations. Then we specify with a lambda what to do with each iteration, knowing that the parallel loop will instantiate tasks on all the cores to execute the iterations. Note how we're using the implicit capture-by-value form of lambda (`[=]`). This means that copies of the GlobalAddresses `A` and `B` will be used in the tasks.

We use the simplest delegate operation: `read` to load the random target index from the B array, and then we use another helper delegate `fetch_and_add` to increment the indicated element in A. 

A minor variant of the above implementation could use `forall<unbound>()` instead, which would allow iterations to be load-balanced, so that if random variation caused some iterations to take longer, other cores could help out by taking some work themselves.

### Localized `forall`
The previous implementation is inefficient in one glaringly obvious way: iterations are scheduled blindly onto cores, without considering what data they will touch. We happen to be iterating consecutively over all the elements in the `B` array. We don't know where each iteration will run, so we must do a remote `read` to fetch `B[i]`. Instead, we could use another form of `forall` to tell the runtime to schedule iterations to line up with elements in the B array:

```cpp
// applications/demos/gups/gups2.cpp
forall(B, FLAGS_sizeB, [=](int64_t& b){
  delegate::fetch_and_add( A + b, 1 );
});
```

In this version, we no longer need to do a remote reference to get an element of `B`, each iteration gets a reference to one automatically. We then just do the one remote delegate call to increment that random element in `A`, wherever it is.

### Asynchronous delegates
Another wasteful practice in the previous implementations is in using a blocking `delegate::fetch_and_add` to increment the remote value. As indicated by the name, `fetch_and_add` doesn't just increment, it also returns the previous value to the caller, which we then promptly ignore. We could save that return trip, and all the nuisance of suspending and waking the calling task, if we invoked a delegate operation that just did the increment asynchronously. Luckily such a call exists:

```cpp
// applications/demos/gups/gups3.cpp
forall(B, FLAGS_sizeB, [=](int64_t& b){
  delegate::increment<async>( A + b, 1);
});
```

This `delegate::increment()` does *not* return the resulting value, which means we can invoke the `async` version of it, using the template parameter. This means we issue the increment delegate, and immediately go on to run the next iteration. To ensure all these updates complete before continuing, we must synchronize with these async delegates. By default, specifying `<async>` enrolls the delegate with the default GlobalCompletionEvent, which is the same GCE that `forall` uses to synchronize itself, so this "magically" means that the enclosing `forall` waits until all increments have completed before waking the *main* task it was called from.
  
This final version of GUPS is doing an amortized single message per iteration (it takes some messages to setup the parallel loops on all cores, and some combined completion messages to detect when the increments are all completed), which is about as good as we can do. The rest is up to the Grappa runtime to perform these messages as efficiently as possible.

### Custom delegate operations
Just as an aside, we "lucked out" in our GUPS implementations above in that we had already had a delegate operation to do the increment. If, instead, GUPS called for some other arbitrary operation, it would actually be nearly as simple to implement that just using the generic `delegate::call()`:

```cpp
// applications/demos/gups/gups4.cpp
forall(B, FLAGS_sizeB, [=](int64_t i, int64_t& b){
  auto addr = A + b;
  delegate::call<async>(addr.core(), [addr, i] {
    *(addr.pointer()) ^= i;
  });
});
```

Here, we use the index in the B array to xor the existing value in A. We compute the address of the element (`addr`), then use it to tell `call` which core to run on (remember because `A` is a linear address, the elements are automatically striped across cores). Finally, on the correct core, we ask `addr` for the local pointer, and use it to update the value. Like `delegate::increment`, `delegate::call` can be made "async" in the same way, and it uses the default GCE, too, so we know that all of these will be complete when the `forall` returns.

Because it is common to do delegates on a single GlobalAddress, we have an alternate form of `call` that takes a GlobalAddress, gets the `core()` from it, and then passes the `pointer()` to the provided lambda. Using this version simplifies the implementation nicely:

```cpp
forall(B, FLAGS_sizeB, [=](int64_t i, int64_t& b){
  delegate::call<async>(A+b, [i](int64_t* a){ *a ^= i; });
});
```

### Irregular parallelism

We will use a tree search to illustrate the spawning and syncing of an unpredictable
number of dynamic tasks. 

In this tutorial you will learn:
- use of `forall_public_*` loops to create tasks that can be load balanced between cores
- use of a `GlobalCompletionEvent` to synchronize dynamic tasks across all cores
