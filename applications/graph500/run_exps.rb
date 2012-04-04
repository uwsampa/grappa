#!/usr/bin/env ruby
require "experiments"


db = "#{ENV['HOME']}/exp/softxmt.db"
table = :graph500

# select between running on XMT or SoftXMT by if it's on cougar
if `hostname`.match /cougar/ then
  cmd = "mtarun -m 2 xmt-csr-local/xmt-csr-local -s %{scale} -e %{edgefactor}"
else
  # command that will be excuted on the command line, with variables in %{} substituted
  cmd = "cd softxmt; make run TARGET=graph.exe ARGS='%{scale} %{edgefactor}'"
end

# map of parameters; key is the name used in command substitution
params = {
  # a: [1,2],
  # b: expr('2*a', 'a**2', '3*a'), # can have expressions using previously defined variables
  # c: 'abc'
  scale: [4, 6, 8, 12],
  edgefactor: [16],
}

# Block that takes the stdout of the shell command and parses it into a Hash
# which will be incorporated into the record inserted into the database.
# If multiple records are desired, an array of Hashes can be returned as well.
# Note: the 'dictionize' method used here is defined by the 'experiments' library 
# and turns the output of named capture groups into a suitable Hash return value.
parser = lambda {|cmdout|
  # /(?<ao>\d+)\s+(?<bo>\d+)\s+(?<co>\w+)/.match(cmdout).dictionize
  h = {}
  cmdout.each_line do |line|
    m = line.chomp.match(/(?<key>[\w_]+):\ (?<value>#{REG_NUM})$/)
    if m then
      h[m[:key]] = m[:value]
    end
  end
  h
}

# function that runs all the experiments
# (note: instead of passing a lambda, an explicit block can be used as well)
# this command also parses the following command-line options if passed to this script:
#  -f,--force      forces experiments to be re-run even if their parameters appear in DB
#  -n,--no-insert  suppresses insertion of records into database
run_experiments(cmd, params, db, table, &parser)
