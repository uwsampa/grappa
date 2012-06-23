sed -i 's/profile_groups\[0\]/GRAPPA_COMM_GROUP/g' profile.*.*.*
sed -i 's/profile_groups\[1\]/GRAPPA_LATENCY_GROUP/g' profile.*.*.*
sed -i 's/profile_groups\[2\]/GRAPPA_USER_GROUP/g' profile.*.*.*
sed -i 's/profile_groups\[3\]/GRAPPA_USERAM_GROUP/g' profile.*.*.*
sed -i 's/profile_groups\[4\]/GRAPPA_SCHEDULER_GROUP/g' profile.*.*.*
sed -i 's/profile_groups\[5\]/GRAPPA_SUSPEND_GROUP/g' profile.*.*.*
sed -i 's/profile_groups\[6\]/GRAPPA_TASK_GROUP/g' profile.*.*.*
sed -i 's/profile_groups\[7\]/GRAPPA_STATE_TIMER_GROUP/g' profile.*.*.*
