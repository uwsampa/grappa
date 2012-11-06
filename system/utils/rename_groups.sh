# Copyright 2010-2012 University of Washington. All Rights Reserved.
# LICENSE_PLACEHOLDER
# This software was created with Government support under DE
# AC05-76RL01830 awarded by the United States Department of
# Energy. The Government has certain rights in the software.

#
# Replacements of Tau profile groups string with the Grappa named group
# This is part of the work around that Tau macro for group definition takes
# the actual expression as the name of a group
#

sed -i 's/profile_groups\[0\]/GRAPPA_COMM_GROUP/g' profile.*.*.*
sed -i 's/profile_groups\[1\]/GRAPPA_LATENCY_GROUP/g' profile.*.*.*
sed -i 's/profile_groups\[2\]/GRAPPA_USER_GROUP/g' profile.*.*.*
sed -i 's/profile_groups\[3\]/GRAPPA_USERAM_GROUP/g' profile.*.*.*
sed -i 's/profile_groups\[4\]/GRAPPA_SCHEDULER_GROUP/g' profile.*.*.*
sed -i 's/profile_groups\[5\]/GRAPPA_SUSPEND_GROUP/g' profile.*.*.*
sed -i 's/profile_groups\[6\]/GRAPPA_TASK_GROUP/g' profile.*.*.*
sed -i 's/profile_groups\[7\]/GRAPPA_STATE_TIMER_GROUP/g' profile.*.*.*
