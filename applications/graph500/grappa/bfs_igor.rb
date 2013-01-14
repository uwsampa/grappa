#!/usr/bin/env ruby

# Copyright 2010-2012 University of Washington. All Rights Reserved.
# LICENSE_PLACEHOLDER
# This software was created with Government support under DE
# AC05-76RL01830 awarded by the United States Department of
# Energy. The Government has certain rights in the software.

require 'igor' 

$GLOG_FLAGS = "GLOG_logtostderr=1 GLOG_v=1"

$GRAPPA_LIBPATH = "/usr/lib:/usr/local/lib:/usr/lib64/openmpi/lib:/sampa/share/boost_1_51_0/lib:/sampa/home/bholt/grappa-beta/tools/built_deps/lib"

$GASNET_FLAGS = "GASNET_BACKTRACE=1 GASNET_FREEZE_SIGNAL=SIGUSR1 GASNET_FREEZE_ON_ERROR=1 GASNET_FREEZE=0 GASNET_NETWORKDEPTH_PP=96 GASNET_NETWORKDEPTH_TOTAL=1024 GASNET_AMCREDITS_PP=48 GASNET_PHYSMEM_MAX=1024M"

Igor.new do
  database '~/exp/grappa-igor.db', :bfs
  cmd = %Q[ make -j mpi_run TARGET=graph.exe NNODE=%{nnode} PPN=%{ppn} BFS=%{bfs}
    SRUN_FLAGS='--time=30:00'
    GARGS='
      --aggregator_autoflush_ticks=%{flushticks}
      --periodic_poll_ticks=%{pollticks}
      --num_starting_workers=%{nworkers}
      --async_par_for_threshold=%{threshold}
      --io_blocks_per_node=%{io_blocks_per_node}
      --io_blocksize_mb=%{io_blocksize_mb}
      --cas_flattener_size=%{cas_flattener_size}
      --global_memory_use_hugepages=%{global_memory_use_hugepages}
      --v=1
      -- -s %{scale} -e %{edgefactor} -pn -%{generator} -f %{nbfs}
    '
  ].gsub(/[\n\r\ ]+/," ")

  # set aside copy of executable and its libraries
  FileUtils.mkdir_p('.igor')
  ldir = ".igor/#{Process.pid}"
  FileUtils.mkdir_p(ldir)
  FileUtils.cp(exe, ldir)
  libs = `ldd graph.exe`
            .split(/\n/)
            .map{|s| s[/.*> (.*) \(.*/,1] }
            .select{|s| s != nil and s != "" and
              not(s[/linux-vdso\.so|ld-linux|^\/lib64/]) }
            .each {|l| FileUtils.cp(l, ldir) }

  prolog {
    sdir = "/scratch/#{__FILE__[/(.*)\.rb/]}.#{Process.pid}"
    FileUtils.mkdir_p(sdir)
  }


%Q[
  mkdir -p /scratch/igor/#{Process.pid};
  for l in #{ldir}/*; do sbcast $l /scratch/igor/#{Process.pid}; done;

  #{$GLOG_FLAGS} LD_LIBRARY_PATH="#{$GRAPPA_LIBPATH}" #{$GASNET_FLAGS}
  srun --resv-ports --cpu_bind=verbose,rank --exclusive --label --kill-on-bad-exit
       --time=30:00 --task-prolog=
/sampa/home/bholt/grappa-beta/applications/graph500/grappa/.srunrc.QGtAUHpT --task-epilog=/sampa/home/bholt/grappa-b
eta/applications/graph500/grappa/.srunrc_epilog.Bapv2Prd --partition grappa --nodes=4 --ntasks-per-node=4 --  ./grap
h.exe  --aggregator_autoflush_ticks=3000000 --periodic_poll_ticks=100000 --num_starting_workers=1024 --async_par_for
_threshold=8 --io_blocks_per_node=1 --io_blocksize_mb=512 --beamer_alpha=20.0 --beamer_beta=20.0 --global_memory_use
_hugepages=true --v=1 --vmodule bfs_beamer=2 -- -s 16 -e 16 -p -K -f 4 

]

end

# command that will be excuted on the command line, with variables in %{} substituted
$machinename = "sampa"
$machinename = "pal" if `hostname` =~ /pal/

huge_pages = ($machinename == "pal") ? false : true

# map of parameters; key is the name used in command substitution
$params = {
  bfs: "bfs_local",
  scale: [23, 26],
  edgefactor: [16],
  generator: "K",
  nbfs: [8],
  nnode: [12],
  ppn: [4],
  nworkers: [1024],
  flushticks: [3000000],
  pollticks:   [100000],
  chunksize: [64],
  threshold: [32],
  io_blocks_per_node: [1],
  io_blocksize_mb: [512],
  nproc: expr('nnode*ppn'),
  machine: [$machinename],
  cas_flattener_size: [24],
  global_memory_use_hugepages: [huge_pages],
}

if __FILE__ == $PROGRAM_NAME
  run_experiments($cmd, $params, $dbpath, $table, &$json_plus_fields_parser)
end
