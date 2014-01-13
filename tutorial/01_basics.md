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

### Individual spawns


Tasks are spawned onto one of two queues: the *private* queue, which resides on a particular core, restricting the task to only run on that same core it was spawned; or the *public* queue, allowing the task to be automatically load-balanced to another core (until it is started on a worker, at which time it is no longer movable). The two functions to do the these spawns are:

- `privateTask([/*capture state*/]{ /* task code */ })`: spawn task to run only on the same core; because it is executed locally, state can be captured by reference if desired
- `publicTask([/*capture state*/]{ /* task code */ })`: spawn a task that may be executed on another core

### Task synchronization
Tasks can currently be synchronized with ("joined") in a couple different ways.

We will cover synchronization more fully in a later section. Tasks may use any of the synchronization primitives directly, but here we will demonstrate just one way: using a `CompletionEvent`. Remember that the Grappa program terminates when the "run" task completes, so if that task does not block on spawned tasks completing, then they may not execute.

```cpp
run([]{
  
  CompletionEvent joiner;
  
  publicTask(&joiner, []{
    LOG(INFO) << "public task ran on " << mycore();
  });
  
  privateTask(&joiner, []{
    LOG(INFO) << "private task ran on " << mycore();
  });
  
  joiner.wait();  
});
```


### Parallel loops

## Bringing it all together: GUPS

We will use a simple benchmark to illustrate the use of parallel loops and delegate operations.


### Irregular parallelism

We will use a tree search to illustrate the spawning and syncing of an unpredictable
number of dynamic tasks. 

In this tutorial you will learn:
- use of `forall_public_*` loops to create tasks that can be load balanced between cores
- use of a `GlobalCompletionEvent` to synchronize dynamic tasks across all cores
