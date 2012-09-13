#!/usr/bin/env ruby
require "experiments"

$dbpath = File.expand_path("~/exp/grappa-sort.db")
$table = :intsort

# command that will be excuted on the command line, with variables in %{} substituted
$cmd = %Q[
  make is NPROCS=%{nproc} CLASS=%{cls};
  salloc --partition=grappa --nodes=%{nnode} --ntasks-per-node=%{ppn}
    mpirun bin/is.%{cls}.%{nproc}
].gsub(/[\n\r\ ]+/," ")
#$cmd = %Q[
  #echo 'class: %{cls}, nnode: %{nnode}, ppn: %{ppn}, scale: %{scale}, buckets: %{log2buckets}, maxkey: %{log2maxkey}'
#].gsub(/[\n\r\ ]+/," ")
machinename = "sampa"
machinename = "pal" if `hostname` =~ /pal/

$classes = {
  "A" => { scale: 23, log2buckets: 10, log2maxkey: 19 },
  "B" => { scale: 25, log2buckets: 10, log2maxkey: 21 },
  "C" => { scale: 27, log2buckets: 10, log2maxkey: 23 },
  "D" => { scale: 29, log2buckets: 10, log2maxkey: 27 },
}

# map of parameters; key is the name used in command substitution
$params = {
  cls: ["A", "B", "C", "D"],
  scale: expr('$classes[cls][:scale]'),
  log2buckets: expr('$classes[cls][:log2buckets]'),
  log2maxkey: expr('$classes[cls][:log2maxkey]'),
  nnode: [4],
  ppn: [4],
  nproc: expr('nnode*ppn'),
  machine: [machinename],
}

$parser = lambda {|cmdout|
  h = {}
  c = Hash.new(0)
  cmdout.each_line do |line|
    pattern = /( Mop\/s\ total\s+=\s+(?<mops_total>[\d.e+-]+) |
                 Time\ in\ seconds\s+=\s+(?<time>[\d.e+-]+)   )/x
    m = line.chomp.match(pattern)
    if m then
      h[m.names[0].to_sym] = m.captures[0]
      puts "+ #{line.chomp}"
    else
      # match statistics
      puts "- #{line.chomp}"
      m = line.chomp.match(/(?<obj>[\w_]+)\ +(?<data>{.*})/m)
      if m then
        obj = m[:obj]
        data = eval(m[:data])
        # puts "#{ap obj}: #{ap data}"
        # sum the fields
        h.merge!(data) {|key,v1,v2| v1+v2 }
      end
    end
  end
  if h.keys.length == 0 then
    puts "Error: didn't find any fields."
  end
  h
}

run_experiments($cmd, $params, $dbpath, $table, &$parser)
