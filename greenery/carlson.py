#!/usr/bin/python

import sys
import re

format = "ncores: (.*); threads per core: (.*); updates per remote: (.*); local bytes: (.*) \(.*\); remote bytes: (.*) \(.*\) total refs: (.*) ref/s: (.*) latency: (.*) sec"


print "ncores, threads per core, updates per remote, local bytes, remote bytes, total refs,  ref/s, latency"
for line in sys.stdin:
    match = re.match(format, line)
    print ", ".join( match.groups() )

