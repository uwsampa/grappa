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
    std::cout << "Exiting root task.\n"
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
