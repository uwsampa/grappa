#!/usr/bin/env ruby
f = open(ARGV[0])
puts "-------"
nedge = f.read(8).unpack("L")[0]
nv = f.read(8).unpack("L")[0]
nadj = f.read(8).unpack("L")[0]
nbfs = f.read(8).unpack("L")[0]
puts "nedge: #{nedge}, nv: #{nv}, nadj: #{nadj}, nbfs: #{nbfs}"

puts "-- edges --"
(0...nedge*2).each{|i|
  puts "#{i}: #{f.read(8).unpack('L')[0]}"
}

puts "-- xoff --"
(0...(2*nv+2)).each{|i|
  puts "#{i}: #{f.read(8).unpack('L')[0]}"
}

puts "-- xadj --"
(0...nadj).each{|i|
  puts "#{i}: #{f.read(8).unpack('L')[0]}"
}

puts "-- bfsroots --"
(0...nbfs).each{|i|
  puts "#{i}: #{f.read(8).unpack('L')[0]}"
}

