#!/usr/bin/env ruby
require 'sequel'

dbpath = ARGV[0]
user_glob = ARGV[1]

puts "database: #{dbpath}"
puts "user_glob: #{user_glob}"

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

Dir.glob(user_glob).each do |f|
  m = f.match(/histogram\.(?<jobid>\d+)\/(?<stat>[\w_]+)\.(?<core>\d+)\.out/)
  r = {jobid:m[:jobid].to_i,core:m[:core].to_i,stat:m[:stat],value:0}
  puts "#{f} -- #{r}"
  data = []
  File.open(f,"r") do |f|
    while b = f.read(8) do
      v = b.unpack("q")[0]
      data << r.merge({value:v})
    end
  end
  histable.multi_insert(data)
end
