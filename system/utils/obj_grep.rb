#!/usr/bin/env ruby

# Copyright 2010-2012 University of Washington. All Rights Reserved.
# LICENSE_PLACEHOLDER
# This software was created with Government support under DE
# AC05-76RL01830 awarded by the United States Department of
# Energy. The Government has certain rights in the software.

require "awesome_print"

#
# Grep an object file for possible active messages
# and print results as <line>,<match>
#

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

