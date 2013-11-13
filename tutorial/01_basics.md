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

## Section 2: Addressing global memory
In Grappa, all memory on all cores is addressable by any other core. This is done using the `GlobalAddress<T>` template class. In the spirit of other Partitioned Global Address Space (PGAS) languages and runtimes, Grappa provides mechanisms for easily distributing data across the various memories of nodes in a cluster.

This section will:

- Describe global addressing, partitioning and the global heap
- Introduce the notion of delegate operations
- Introduce logging and command-line flag utilities

### Global addresses
Global addresses fall into one of three different categories:

- 2D addresses
- Linear addresses
- Symmetric addresses

#### 2D Addresses

The first, 2D addresses, are the simplest. They represent just the pairing of a pointer, and the Core where that pointer is valid. These can be created using the call `make_global()`, and will typically reference either data on a worker's execution stack, or something allocated from the core-private heap (using `new`). Consider the following example (note: the rest of `main()` has been left out, see the full source at `tutorial/addressing.cpp`):

```cpp
long count = 0;
GlobalAddress<long> g_count = make_global( &count );
LOG(INFO) << "global address -- valid on core:" << g_count.core() << ", pointer:" << g_count.pointer();
```

The call to `make_global()` creates a new global address with the given pointer to the `count` variable on this task's stack and encodes the core where it is valid (`Grappa::mycore()`) with it.

#### Global heap & linear addresses
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
