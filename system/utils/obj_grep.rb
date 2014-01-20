#!/usr/bin/env ruby

########################################################################
## This file is part of Grappa, a system for scaling irregular
## applications on commodity clusters. 

## Copyright (C) 2010-2014 University of Washington and Battelle
## Memorial Institute. University of Washington authorizes use of this
## Grappa software.

## Grappa is free software: you can redistribute it and/or modify it
## under the terms of the Affero General Public License as published
## by Affero, Inc., either version 1 of the License, or (at your
## option) any later version.

## Grappa is distributed in the hope that it will be useful, but
## WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## Affero General Public License for more details.

## You should have received a copy of the Affero General Public
## License along with this program. If not, you may obtain one from
## http://www.affero.org/oagpl.html.
########################################################################

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

