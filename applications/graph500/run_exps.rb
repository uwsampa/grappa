#!/usr/bin/env ruby
require "experiments"

db = "#{ENV['HOME']}/exp/softxmt.db"
table = :graph500

# select between running on XMT or SoftXMT by if it's on cougar
if `hostname`.match /cougar/ then
  cmd = "mtarun -m %{nproc} xmt-csr-local/xmt-csr-local -s %{scale} -e %{edgefactor}"
  machinename = "cougarxmt"
else
  # command that will be excuted on the command line, with variables in %{} substituted
  # cmd = "cd softxmt && make run TARGET=graph.exe ARGS='--v=0 --num_starting_workers=%{nworkers} -- -s %{scale} -e %{edgefactor} -p' NPROC=%{nproc} NNODE=%{nnode} PPN=%{ppn}"
  cmd = %Q< cd softxmt && GLOG_logtostderr=1 LD_LIBRARY_PATH="\$LD_LIBRARY_PATH:/usr/local/lib:/sampa/home/bholt/opt/lib:/usr/lib64/openmpi-psm/lib:/sampa/home/bholt/opt/lib:/sampa/share/gflags/lib:/sampa/share/glog/lib:/usr/lib:/usr/lib:/sampa/share/gperftools-2.0/lib" GASNET_NUM_QPS=3 \
  srun --resv-ports --cpu_bind=verbose,rank --exclusive --label --kill-on-bad-exit --task-prolog ~/srunrc.all --partition softxmt --nodes=%{nnode} --ntasks-per-node=%{ppn} -- ./graph.exe --v=0 --num_starting_workers=%{nworkers} --aggregator_autoflush_ticks=%{flushticks} --flush_on_idle=%{flush_on_idle} -- -s %{scale} -e %{edgefactor} -p >
  
  machinename = "sampa"
end

# map of parameters; key is the name used in command substitution
params = {
  scale: [18],
  edgefactor: [16],
  nworkers: [256, 384, 512, 1024, 2048],
  nnode: [2, 4],
  ppn: [2, 4, 6, 8],
  flushticks: [100000, 500000, 1000000],
  flush_on_idle: [0, 1],
  nproc: expr('nnode*ppn'),
  machine: [machinename],
}

def inc_avg(avg, count, val)
  return avg + (val-avg)/count
end

# Block that takes the stdout of the shell command and parses it into a Hash
# which will be incorporated into the record inserted into the database.
# If multiple records are desired, an array of Hashes can be returned as well.
# Note: the 'dictionize' method used here is defined by the 'experiments' library 
# and turns the output of named capture groups into a suitable Hash return value.
parser = lambda {|cmdout|
  # /(?<ao>\d+)\s+(?<bo>\d+)\s+(?<co>\w+)/.match(cmdout).dictionize
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
        # do incremental average if key is found in map already
        h.merge!(data) {|key,v1,v2| inc_avg(v1, c[key]+=1, v2) }
      end
    end
  end
  if h.keys.length == 0 then
    puts "Error: didn't find any fields."
  end
  h
}

# function that runs all the experiments
# (note: instead of passing a lambda, an explicit block can be used as well)
# this command also parses the following command-line options if passed to this script:
#  -f,--force      forces experiments to be re-run even if their parameters appear in DB
#  -n,--no-insert  suppresses insertion of records into database
run_experiments(cmd, params, db, table, &parser)
