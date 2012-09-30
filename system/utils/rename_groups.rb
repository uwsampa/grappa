#!/usr/bin/env ruby

# Copyright 2010-2012 University of Washington. All Rights Reserved.
# LICENSE_PLACEHOLDER
# This software was created with Government support under DE
# AC05-76RL01830 awarded by the United States Department of
# Energy. The Government has certain rights in the software.

require "open4"

#
# In all profile.*.*.* files rename occurances of group id number with the name
#

cmd_template = "sed -i 's/%{id}/%{name}/g' profile.*.*.*"

STDIN.read.split("\n").each do |line|
    name,id = line.chomp().split(/\s+/)
    id = id.gsub(/\[/, "\\[").gsub(/\]/, "\\]")
    d = {id:id, name:name} 
    cmd = cmd_template % d
    puts cmd
    cmderr = ""
    status = Open4::popen4(cmd) do |pid, stdin, stdout, stderr|
        cmderr = stderr.read.strip
    end
    if not(status) then
        puts "error! #{status}\n#{cmderr}"
        exit(1)
    end
end 
