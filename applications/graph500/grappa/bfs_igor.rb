#!/usr/bin/env ruby

# Copyright 2010-2012 University of Washington. All Rights Reserved.
# LICENSE_PLACEHOLDER
# This software was created with Government support under DE
# AC05-76RL01830 awarded by the United States Department of
# Energy. The Government has certain rights in the software.

require 'igor' 
require_relative '../../../igor_common.rb'

Igor do
  database '~/exp/grappa-igor.db', :bfs

  # parser set by igor_common.rb

  # set aside copy of executable and its libraries
  # ldir = "/scratch/#{ENV['USER']}/igor/#{Process.pid}"
  ldir = "#{File.expand_path File.dirname(__FILE__)}/.igor/#{Process.pid}"
  FileUtils.mkdir_p(ldir)
  FileUtils.cp('graph.exe', ldir)
  libs = `bash -c "LD_LIBRARY_PATH=#{$GRAPPA_LIBPATH} ldd graph.exe"`
            .split(/\n/)
            .map{|s| s[/.*> (.*) \(.*/,1] }
            .select{|s| s != nil and s != "" and
              not(s[/linux-vdso\.so|ld-linux|^\/lib64/]) }
            .each {|l| FileUtils.cp(l, ldir) }
  # copy system mpirun
  # FileUtils.cp(`which mpirun`, "#{ldir}/mpirun")
  `cp $(which mpirun) #{ldir}/mpirun`
  puts 'done with setup'

  tdir = "/scratch/#{ENV['USER']}/igor/#{Process.pid}"

  command %Q[
    if [[ ! -d "#{tdir}" ]]; then 
      srun mkdir -p #{tdir};
      ls #{tdir};
      echo $(hostname);
      for l in $(ls #{ldir}); do
        echo $l; sbcast #{ldir}/$l #{tdir}/${l};
      done;
    fi;

    #{$GLOG_FLAGS} LD_LIBRARY_PATH="#{tdir}:#{$GRAPPA_LIBPATH}" #{$GASNET_FLAGS} mpirun -npernode 1 ldd #{tdir}/graph.exe;

    mpirun -npernode 1 bash -c "ipcs -m | grep #{ENV['USER']} | cut -d\\  -f1 | xargs -n1 -r ipcrm -M";
    #{$GLOG_FLAGS} LD_LIBRARY_PATH="#{tdir}:#{$GRAPPA_LIBPATH}" #{$GASNET_FLAGS}
    mpirun #{tdir}/graph.exe
              --aggregator_autoflush_ticks=%{flushticks}
              --periodic_poll_ticks=%{pollticks}
              --num_starting_workers=%{nworkers}
              --async_par_for_threshold=%{threshold}
              --io_blocks_per_node=%{io_blocks_per_node}
              --io_blocksize_mb=%{io_blocksize_mb}
              --global_memory_use_hugepages=%{global_memory_use_hugepages}
              --v=1
              -- -s %{scale} -e %{edgefactor} -pn -%{generator} -f %{nbfs}
  ].gsub($/," ")
  
  params {
    bfs "bfs_local"
    scale 23
    edgefactor 16
    generator "K"
    nbfs 8
    nnode 4
    ppn 4
    nworkers 1024
    flushticks 3000000
    pollticks 100000
    chunksize 64
    threshold 32
    io_blocks_per_node 1
    io_blocksize_mb 512
    cas_flattener_size 24
    global_memory_use_hugepages 1
  }

  run

end

Igor.pry
