Running Grappa programs
===============================================================================
Grappa jobs are run like any other MPI job. This process varies depending on how your cluster is configured and what version of MPI you use. The Grappa team primarily works on machines that use the Slurm scheduler, and thus we have built tools to make it easy to run Grappa programs under Slurm. However, it is not necessary to use these tools to run Grappa jobs---as long as you make sure the right environment variables are set, any MPI job launcher will work.

Using Slurm directly
-------------------------------------------------------------------------------
Most of the machine we run on use Slurm as their job scheduler. You 

### Running an application: ###
```bash
# in build/Make+Release: #
# build the desired executable #
make -j graph_new.exe
# launch a Slurm job to run it: #
srun --nodes=2 --ntasks-per-node=2 -- graph_new.exe --scale=26 --bench=bfs --nbfs=8 --num_starting_workers=512
```

### Running a Grappa test: ###
```bash
# in your configured directory, (e.g. build/Make+Release) #
# (builds and runs the test) #
make -j check-New_delegate_tests
# or in two steps: #
make -j New_delegate_tests.test
srun --nodes=2 --ntasks-per-node=2 -- system/New_delegate_tests.test
```

Using Slurm and `grappa_srun` (deprecated)
-------------------------------------------------------------------------------
The Ruby script `bin/grappa_srun` can be used to automatically manage the pesky `srun` flags and environment variables that are specific to a few machines used by teh Grappa team. We don't maintain the script anymore, so this probably won't be useful to you; we've documented it here for posterity. Try running `bin/grappa_srun --help` for more detailed usage information.

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
You may use whatever other mechanism you normally do to launch MPI jobs as well. You may want to set environment variables as set in ```util/env.sh``` or ```bin/env.sh``` (which will be automatically set if you ```source <GRAPPA_PREFIX>/bin/settings.sh```), but they shouldn't make a big difference.
