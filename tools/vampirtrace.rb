#!/usr/bin/env ruby
require 'fileutils'; include FileUtils
require 'optparse'; require 'ostruct'

def `(cmd)
  system cmd
  if not $?.success?
    warn "error! debugging..."
    require 'pry'; binding.pry
  end
end

opt = OpenStruct.new
opt.prefix = '/opt/vampir'

OptionParser.new {|p|
  p.on('--prefix=path'){|p| opt.prefix = p }
}.parse!

`wget http://sampa.cs.washington.edu/grappa/VampirTrace-5.14.4.tar.gz`
`tar xzf VampirTrace-5.14.4.tar.gz`

cd ("VampirTrace-5.14.4") do
  `./configure --prefix=#{opt.prefix}`
  `make -j4`
  `make install`
end

rmdir "VampirTrace-5.14.4"
rm "VampirTrace-5.14.4.tar.gz"
