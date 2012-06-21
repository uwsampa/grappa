#!/usr/bin/env ruby

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

