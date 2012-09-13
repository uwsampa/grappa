#!/usr/bin/env ruby
require "experiments"

$dbpath = File.expand_path("~/exp/grappa-sort.db")
$table = :intsort

# command that will be excuted on the command line, with variables in %{} substituted
$cmd = %Q[ make mpi_run TARGET=sort.exe NNODE=%{nnode} PPN=%{ppn}
  SRUN_FLAGS='--time=10:00'
  GARGS='
    --aggregator_autoflush_ticks=%{flushticks}
    --periodic_poll_ticks=%{pollticks}
    --num_starting_workers=%{nworkers}
    --async_par_for_threshold=%{threshold}
    --v=0
    -- -s %{scale} -b %{log2buckets}
  '
].gsub(/[\n\r\ ]+/," ")
machinename = "sampa"
machinename = "pal" if `hostname` =~ /pal/

# map of parameters; key is the name used in command substitution
$params = {
  scale: [30],
  log2buckets: [7],
  nnode: [2, 4, 8, 16],
  ppn: [4, 6, 8],
  nworkers: [4096],
  flushticks: [2000000],
  pollticks: [20000],
  chunksize: [64],
  threshold: [64],
  nproc: expr('nnode*ppn'),
  machine: [machinename],
}

$parser = lambda {|cmdout|
  h = {}
  c = Hash.new(0)
  cmdout.each_line do |line|
    m = line.chomp.match(/(?<key>[\w_]+):\ (?<value>#{REG_NUM})$/)
    if m then
      h[m[:key].downcase.to_sym] = m[:value].to_f
    else
      # match statistics
      puts "trying to match statistics...\n#{line.chomp}"
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
