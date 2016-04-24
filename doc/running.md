Running Grappa programs
===============================================================================
Grappa jobs are run like any other MPI job. This process varies depending on how your cluster is configured and what version of MPI you use. Here are some examples.

Note that running MPI jobs like Grappa is very system-dependent. Talk to your system administrator to learn the correct commands for running process-per-core jobs like Grappa apps.

Slurm with srun
---------------

Most of the systems we run on allow MPI jobs to be launched with ```srun``` directly:

```bash
srun --nodes=2 --ntasks-per-node=2 -- applications/demos/hello_world/hello_world.exe
```

Slurm with salloc/mpirun
------------------------

Some MPI installations aren't configured to work with srun; instead, they require first getting an "allocation" from the scheduler and then calling "mpirun" or "mpiexec" inside that allocation:

```bash
salloc --nodes=2 --ntasks-per-node=2 -- mpirun applications/demos/hello_world/hello_world.exe
```

Mpirun with hostfile
------------------------

Some MPI installations aren't configured to work with srun; instead, they require first getting an "allocation" from the scheduler and then calling "mpirun" or "mpiexec" inside that allocation. On these systems you have to make sure the task count and host count are appropriate to get you whatever number of tasks per node you expect.

```bash
mpirun -n 4 -hosts=<list of 2 hosts> applications/demos/hello_world/hello_world.exe
```

Legacy information: using Slurm and `grappa_srun`
-------------------------------------------------------------------------------
The Grappa team built a tool with some shortcuts for running on machines they commonly use during the development process. It's not necessary to use this tool to run Grappa jobs---any MPI job launcher will work. These days, we usually just use ```srun``` directly.

But for posterity, here's more info: the Ruby script `bin/grappa_srun` can be used to automatically manage the pesky `srun` flags that are needed to run Grappa programs correctly on a number of machines that we've tested on. Try running `bin/grappa_srun --help` for more detailed usage information.

### Running an application: ###
```bash
# in build/Make+Release: #
# build the desired executable #
make -j graph_new.exe
# launch a Slurm job to run it: #
bin/grappa_srun --nnode=2 --ppn=2 -- graph_new.exe --scale=26 --bench=bfs --nbfs=8 --num_starting_workers=512
```

### Running a Grappa test: ###
```bash
# in your configured directory, (e.g. build/Make+Release) #
# (builds and runs the test) #
make -j check-New_delegate_tests
# or in two steps: #
make -j New_delegate_tests.test
bin/grappa_srun --nnode=2 --ppn=2 --test=New_delegate_tests
```
