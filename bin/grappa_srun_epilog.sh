#!/bin/bash
for i in `ipcs -m | grep bholt | cut -d" " -f1`; do ipcrm -M $i; done
