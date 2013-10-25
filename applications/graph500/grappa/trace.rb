#!/usr/bin/env ruby
require 'pty'
require 'fileutils'; include FileUtils

cmd = ARGV.join(' ')
begin
  PTY.spawn(cmd) do |stdin, stdout, pid|
    begin
      stdin.sync
      stdin.each{|line| puts line.strip}
    rescue Errno::EIO
    end
  end
rescue PTY::ChildExited
end

otf = Dir.glob("*.otf").max_by {|f| File.mtime(f)}
base = otf[/(.*)\.otf/,1]
open("#{base}.sh", "w"){|f| f.write("#{cmd}\n") }
dest = "trace/#{base}"
mkdir dest
`mv #{base}.* #{dest}`
