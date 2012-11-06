#!/usr/bin/env ruby

# Copyright 2010-2012 University of Washington. All Rights Reserved.
# LICENSE_PLACEHOLDER
# This software was created with Government support under DE
# AC05-76RL01830 awarded by the United States Department of
# Energy. The Government has certain rights in the software.

#
# Print unique lines. Like uniq
#

d = {}
STDIN.read.split("\n").each do |line|
    val = line.chomp()
    cnt = d[val] 
    if cnt then
        d[val] = cnt+1
    else
        d[val] = 1
    end
end

d.keys().each do |k|
    print k + "\n"
end

