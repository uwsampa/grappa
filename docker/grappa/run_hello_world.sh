#!/bin/bash
# have to make sure we re-set shmmax before running any Grappa programs
sysctl -w kernel.shmmax=$((1<<30))

# manually invoking mpirun
# reduce stack size and use fewer workers to reduce memory usage
exec mpirun --allow-run-as-root -n 2 /grappa/build/Ninja+Release/applications/demos/hello_world.exe --stack_size=$((1<<16)) --num_starting_workers=4 --logtostderr
