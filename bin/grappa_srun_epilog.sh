#!/bin/bash
# for i in `ipcs -m | grep bholt | cut -d" " -f1`; do ipcrm -M $i; done
ipcs -m | grep $USER | cut -d\  -f1 | xargs -n1 -r ipcrm -M
rm -f /dev/shm/GrappaLocaleSharedMemory
