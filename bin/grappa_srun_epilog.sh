#!/bin/bash
# for i in `ipcs -m | grep bholt | cut -d" " -f1`; do ipcrm -M $i; done
ipcs -m | grep $USER | awk '{print $2}' | xargs -n1 -r ipcrm -m
rm -f /dev/shm/GrappaLocaleSharedMemory
