#!/usr/bin/env ruby
require 'sequel'

dbpath = ARGV[0]
histdir = ARGV[1]

jobid = histdir.match(/.*\.(?<job>\d+)/)[:job].to_i

puts "database: #{dbpath}"
puts "reading histograms from #{histdir}"
puts "jobid: #{jobid}"

db = Sequel.sqlite(dbpath)
table = :histograms
histable = db[table]

db.create_table?(table){
  primary_key :id
  Integer     :jobid
  Integer     :core
  String      :stat
  Integer     :value
  index :jobid
  index :stat
}

Dir.glob("#{histdir}/*.0.out").each do |stat0|
  statname = stat0.match(/.*\/(?<stat>.*?)\./)[:stat]
  puts "## stat: #{statname}"
  
  data = []
  
  Dir.glob("#{histdir}/#{statname}.*.out").each do |statcore|
    core = statcore.match(/.*\.(?<core>\d+)\.out/)[:core]
    puts "#### #{statname}<#{core}>"
    File.open("#{histdir}/#{statname}.#{core}.out","r") do |f|
      while b = f.read(8) do
        v = b.unpack("q")[0]
        data << {jobid:jobid,core:core,stat:statname,value:v}
      end
    end
  end
  
  histable.multi_insert(data)
end
