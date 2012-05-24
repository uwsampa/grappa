#!/usr/bin/env ruby
require "awesome_print"

obj_fn = ARGV[0]

STDIN.read.split("\n").each do |line|
    asHexStr = line.to_i().to_s(16)
    pattern = /#{Regexp.quote(asHexStr)}/

    File.open(obj_fn) do |file|
        matches = file.each().grep(pattern)
        am_matches = matches.each().grep(/_am|am_/)

        if am_matches.empty? then
            outl = ["(no AM)"] + matches
            ap [line,outl]
        else
            ap [line,am_matches]
        end
    end
end

