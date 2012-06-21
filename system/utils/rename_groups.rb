#!/usr/bin/env ruby

require "open4"

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
