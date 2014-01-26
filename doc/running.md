Running Grappa programs
===============================================================================
Grappa jobs are run like any other MPI job. This process varies depending on how your cluster is configured and what version of MPI you use. The Grappa team primarily works on machines that use the Slurm scheduler, and thus we have built a tools to make it easy to run Grappa programs under Slurm. However, it is not necessary to use these tools to run Grappa jobs---as long as you make sure the right environment variables are set, any MPI job launcher will work.

Using Slurm and `grappa_srun`
-------------------------------------------------------------------------------
The Ruby script `bin/grappa_srun` can be used to automatically manage the pesky `srun` flags and GASNET settings that are needed to run Grappa programs correctly on a number of machines that we've tested on. Try running `bin/grappa_srun --help` for more detailed usage information.

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

Using another MPI job launcher
-------------------------------------------------------------------------------
You may use whatever mechanism you normally do to launch MPI jobs, but make sure you set the appropriate environment variables. Take a look at `bin/srun_prolog` and `bin/srun_epilog` to see what we do for Slurm.
