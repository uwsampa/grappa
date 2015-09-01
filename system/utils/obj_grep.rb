#!/usr/bin/env ruby

########################################################################
## Copyright (c) 2010-2015, University of Washington and Battelle
## Memorial Institute.  All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions
## are met:
##     * Redistributions of source code must retain the above
##       copyright notice, this list of conditions and the following
##       disclaimer.
##     * Redistributions in binary form must reproduce the above
##       copyright notice, this list of conditions and the following
##       disclaimer in the documentation and/or other materials
##       provided with the distribution.
##     * Neither the name of the University of Washington, Battelle
##       Memorial Institute, or the names of their contributors may be
##       used to endorse or promote products derived from this
##       software without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
## "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
## LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
## FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
## UNIVERSITY OF WASHINGTON OR BATTELLE MEMORIAL INSTITUTE BE LIABLE
## FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
## CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
## OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
## BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
## LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
## (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
## USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
## DAMAGE.
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

