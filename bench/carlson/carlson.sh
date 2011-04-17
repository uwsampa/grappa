#!/bin/csh

setenv GOMP_CPU_AFFINITY "0-11:2"
#GOMP_CPU_AFFINITY="0-11:2" hugectl --heap -- numactl --membind=0  -- ./carlson 6 16 0 23 0 1 1 1
setenv BINARY '/usr/local/bin/hugectl --heap -- numactl --membind=0 -- ./carlson'

setenv remote_size 24

foreach repetitions (1 2 3 4 5)

#foreach local_random_access (0 1)
foreach local_random_access (1)

foreach total_footprint_log (5 6 7 8 9 10 11 12 13 14 15 16) # 17 18 19 20 21 22 23 24)
#foreach total_footprint_log ( 5 15 6 16 7 17 8 18 9 19 10 20 11 21 12 22 13 23 14 )
#foreach total_footprint_log (24)
#foreach total_footprint_log (9 10)

#foreach updates_per_remote (0 32 1 16 2 8 4)
foreach updates_per_remote (0 1 2 4 8 16 32)
#foreach updates_per_remote (32)
#foreach updates_per_remote (0 1)


# # baseline
# foreach num_cores (1)
# foreach threads_per_core (1)
# foreach use_local_prefetch_and_switch (0)
# foreach issue_remote_reference (0)
# foreach use_remote_reference (1)
#  $BINARY $num_cores $threads_per_core $updates_per_remote $total_footprint_log \
#     $use_local_prefetch_and_switch $local_random_access \
#     $issue_remote_reference $use_remote_reference $remote_size
# end
# end
# end
# end
# end

# # max
# foreach num_cores (1)
# foreach threads_per_core (1)
# foreach use_local_prefetch_and_switch (0)
# foreach issue_remote_reference (0)
# foreach use_remote_reference (0)
#  $BINARY $num_cores $threads_per_core $updates_per_remote $total_footprint_log \
#     $use_local_prefetch_and_switch $local_random_access \
#     $issue_remote_reference $use_remote_reference $remote_size
# end
# end
# end
# end
# end

foreach num_cores (1 6)

#foreach threads_per_core (1 2 3 4 5 6 7 8)
#foreach threads_per_core (1 2 4 8 16 32 64 128 256)
foreach threads_per_core (1 2 4 8 12 16 32 64)
#foreach threads_per_core (64 128 256)
foreach use_local_prefetch_and_switch (0)

foreach issue_remote_reference (1)
foreach use_remote_reference (1)
    echo repetition: $repetitions
 $BINARY \
    --cores=$num_cores \
    --threads_per_core=$threads_per_core \
    --updates_per_remote=$updates_per_remote \
    --local_footprint_log=$total_footprint_log \
    --local_prefetch_and_switch=$use_local_prefetch_and_switch \
    --local_random_access=$local_random_access \
    --issue_remote_reference=$issue_remote_reference \
    --use_remote_reference=$use_remote_reference \
    --remote_footprint_log=$remote_size
end
end
end
end

end
end
end
end

end
