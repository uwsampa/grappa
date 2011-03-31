#!/bin/bash

experiment_id=`date +%s`

make inc2.exe

#echo num_cores, iters, racy_runtime_ns, racy_runtime_per_iter, racy_runtime_per_inc, racy_result, atomic_runtime_ns, atomic_runtime_per_iter, atomic_runtime_per_inc, atomic_result | tee inc-$experiment_id.log

#echo num_cores, iters, racy_runtime_ns, racy_runtime_per_iter, racy_runtime_per_inc, racy_throughput, racy_result, racy_sum, atomic_runtime_ns, atomic_runtime_per_iter, atomic_runtime_per_inc, atomic_throughput, atomic_result, atomic_sum, cas_runtime_ns, cas_runtime_per_iter, cas_runtime_per_inc, cas_throughput, cas_result, cas_sum | tee inc-$experiment_id.log

echo num_cores, iters, model, runtime_ns, runtime_per_iter, runtime_per_inc, throughput, result, sum | tee inc-$experiment_id.log

for reps in 1 2 3 4 5 6 7 8 9 10
do
    for iters_log in 26
    do
	for num_cores in 1 2 3 4 5 6 7 8 9 10 11 12
	do
	    numactl --membind=0 -- ./inc2.exe --racy_single --racy_with_unrelated_racy --racy_with_unrelated_atomic --atomic_with_unrelated_atomic --atomic_with_unrelated_racy  --unrelated_with_unrelated_atomic  --racy_with_unrelated_delegate --num_cores $num_cores --iters_log $iters_log
	done
    done
done | tee -a inc-$experiment_id.log


