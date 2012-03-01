#!/usr/bin/env ruby
require "/sampa/home/bdmyers/experiments/lib/experiments"
require "/sampa/home/bdmyers/experiments/lib/parsers"

db = "data.db"
table = :treegen

t1 = "-t 1 -a 3 -d 10 -b 4 -r 19"

cmd=   "OMPI_MCA_btl_sm_use_knem=0 \\      
        LD_LIBRARY_PATH=':/usr/mpi/qlogic/lib64:/sampa/share/gflags/lib:/sampa/share/glog/lib:/usr/lib:/sampa/share/gflags/lib' \\
        mpirun -l -x LD_LIBRARY_PATH -H n01,n04 -np %{num_procs} -- ./TreeGen.exe -- %{tree} -T %{num_threads} -c %{chunk_size} -i %{poll_interval} -P %{num_places} -p %{num_procs}"


params = {
    tree: [t1],
    num_threads: [16, 32, 64, 72, 78, 84, 96, 100, 101, 102, 103, 104, 108, 118, 128, 256, 512],
    chunk_size: [1, 2, 3, 4, 5, 6, 8, 10, 16, 20, 50],
    poll_interval: [1],
    num_places: [1],
    num_procs: [2],
    trial: [1,2,3]
}

run_experiments(cmd, params, db, table, &$rDictParser)


