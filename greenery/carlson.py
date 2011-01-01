#!/usr/bin/python

import sys
import re

format = "ncores: (.*); threads per core: (.*); updates per remote: (.*); total bytes: (.*) local bytes: (.*) \(.*\); remote bytes: (.*) \(.*\) total refs: (.*)  localpns: (.*) lra: (.*) irr: (.*) urr: (.*) ref/s: (.*) latency: (.*) sec"


print "ncores, threads per core, updates per remote, total bytes, local bytes, remote bytes, total refs, localpns, lra, irr, urr, ref/s, latency"
for line in sys.stdin:
    match = re.match(format, line)
    if match is not None:
        print ", ".join( match.groups() )

