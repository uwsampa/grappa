Running Grappa programs
===============================================================================
The Grappa team does its development on machines 

Using Slurm and `grappa_srun.rb`
-------------------------------------------------------------------------------
The Ruby script `bin/grappa_srun.rb` can be used to automatically manage the pesky `srun` flags and GASNET settings that are needed to run Grappa programs correctly on a number of machines that we've tested on. Try running `bin/grappa_srun.rb --help` for more detailed usage information.

### Running an application: ###
```bash
# in build/Make+Release: #
# build the desired executable #
make -j graph_new.exe
# launch a Slurm job to run it: #
bin/grappa_srun.rb --nnode=2 --ppn=2 -- graph_new.exe --scale=26 --bench=bfs --nbfs=8 --num_starting_workers=512
```

### Running a Grappa test: ###
```bash
# in your configured directory, (e.g. build/Make+Release) #
# (builds and runs the test) #
make -j check-New_delegate_tests
# or in two steps: #
make -j New_delegate_tests.test
bin/grappa_srun.rb --nnode=2 --ppn=2 --test=New_delegate_tests
```

Using another MPI job launcher
-------------------------------------------------------------------------------
You may use whatever mechanism you normally do to launch MPI jobs, but make sure you set the appropriate environment variables. Take a look at `bin/srun_prolog` and `bin/srun_epilog` to see what we do for Slurm.
