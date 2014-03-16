#!/usr/bin/env ruby
require 'igor'
require_relative_to_symlink '../igor_db'

# find source directory (assumes this is symlinked into )
DIR = with(File){expand_path(dirname(symlink?(__FILE__) ? readlink(__FILE__) : __FILE__))}

Igor do
  
  # database '~/exp/oopsla14.sqlite', :gups
  database(table: :gups)
  
  GFLAGS.merge!({
    v: 0,
    log_array_size: 31,
    log_iterations: 28,
  })
  GFLAGS.delete :flat_combining
  
  params.merge!(GFLAGS)
    
  @c = ->{ %Q[
    grappaclang-no-opt -O3 -DCOST_EXPERIMENT %{compile_flags} -DDATA_SIZE=%{data}
      -o .igor/gups.%{name}.%{data}.exe #{DIR}/gups.cpp && echo '-- compile done' &&
    grappa_srun --no-freeze-on-error -- .igor/gups.%{name}.%{data}.exe --metrics #{GFLAGS.expand}
  ].gsub(/\s+/,' ') }
  command @c[]
  
  sbatch_flags << "--time=10:00"
  
  params {
    nnode 8; ppn 8
    stay true,false
    compile_flags expr("stay ? '-DBLOCKING' : ''")
    name expr("stay ? 'stay' : 'go'")
    data 1,8,32
  }
  
  expect :gups_runtime
  ignore :command
  
  interact # enter interactive mode
end
