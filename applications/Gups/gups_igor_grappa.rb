#!/usr/bin/env ruby
require 'igor'

# inherit parser, sbatch_flags
load '../igor_common.rb'

def expand_flags(*names)
  names.map{|n| "--#{n}=%{#{n}}"}.join(' ')
end

Igor do
  include Isolatable

  database '~/exp/sosp.db', :bfs

  # isolate everything needed for the executable so we can sbcast them for local execution
  isolate(['Gups_tests.test'],
    File.dirname(__FILE__))

  params.merge!(GFLAGS)

  command %Q[ %{tdir}/grappa_srun.rb
    --test Gups_tests --
    #{expand_flags(*GFLAGS.keys)}
    --load_balance=none
    --sizeA=1073741824
    --iterations=4294967296
    --load_balance=none
    --num_starting_workers=512
    --aggregator_autoflush_ticks=100000
    --periodic_poll_ticks=20000
    --global_memory_use_hugepages=false
    --repeats=1
    --async_par_for_threshold=1
    --rdma=true
    --rdma=true
    --validate=false
    --outstanding=4096
    --rdma_flush_on_idle=true
  ].gsub(/\s+/,' ')

  sbatch_flags << "--time=10:00"

  params {

    num_starting_workers 512
    stack_size 2**16

    aggregator_autoflush_ticks 100000
    periodic_poll_ticks        20000

    load_balance    'none'

    rdma_workers_per_core 2**4
    rdma_buffers_per_core 12

    shared_pool_size 2**20

    poll_on_idle 1
    flush_on_idle 1
    rdma_flush_on_idle 1
    global_memory_use_hugepages 0

    nnode       64, 4, 8, 16, 32
    ppn         16

    async_par_for_threshold 1

    iterations              2**32
    repeats                 1
    rdma                    1
    sizeA                   2**30
    validate                0
    outstanding             2**12

    nelsontag   'gups-scaling2-pal'
    tag         'none'
  }

  expect :UserStats_updates_per_s

  interact # enter interactive mode
end
