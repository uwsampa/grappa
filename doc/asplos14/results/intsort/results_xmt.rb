results = [
  {
    machine: 'xmt',
    version: 'xmt',
    problem: 'E',
    nnode: 124,
    ppn: 1,
    iterations: 10,
    problem_size: 2147483648,
    run_time: 12.40,
    mops_total: 1731.88
  },{
    machine: 'xmt',
    version: 'xmt',
    problem: 'E',
    nnode: 112,
    ppn: 1,
    iterations: 10,
    problem_size: 2147483648,
    run_time: 13.37,
    mops_total: 1606.61
  },{
    machine: 'xmt',
    version: 'xmt',
    problem: 'E',
    nnode: 96,
    ppn: 1,
    iterations: 10,
    problem_size: 2147483648,
    run_time: 13.15,
    mops_total: 1633.53
  },{
    machine: 'xmt',
    version: 'xmt',
    problem: 'E',
    nnode: 80,
    ppn: 1,
    iterations: 10,
    problem_size: 2147483648,
    run_time: 12.90,
    mops_total: 1664.23
  },{
    machine: 'xmt',
    version: 'xmt',
    problem: 'E',
    nnode: 64,
    ppn: 1,
    iterations: 10,
    problem_size: 2147483648,
    run_time: 9.51,
    mops_total: 2257.62
  },{
    machine: 'xmt',
    version: 'xmt',
    problem: 'E',
    nnode: 48,
    ppn: 1,
    iterations: 10,
    problem_size: 2147483648,
    run_time: 10.90,
    mops_total: 1969.96
  },{
    machine: 'xmt',
    version: 'xmt',
    problem: 'E',
    nnode: 32,
    ppn: 1,
    iterations: 10,
    problem_size: 2147483648,
    run_time: 15.58,
    mops_total: 1378.80
  },{
    machine: 'xmt',
    version: 'xmt',
    problem: 'E',
    nnode: 16,
    ppn: 1,
    iterations: 10,
    problem_size: 2147483648,
    run_time: 29.99,
    mops_total: 716.10
  },{
    machine: 'xmt',
    version: 'xmt',
    problem: 'E',
    nnode: 1,
    ppn: 1,
    iterations: 10,
    problem_size: 2147483648,
    run_time: 466.65,
    mops_total: 46.02
  },{
    machine: 'xmt',
    version: 'xmt',
    problem: 'E',
    nnode: 40,
    ppn: 1,
    iterations: 10,
    problem_size: 2147483648,
    run_time: 12.69,
    mops_total: 1691.87
  },{
    machine: 'xmt',
    version: 'xmt',
    problem: 'E',
    nnode: 28,
    ppn: 1,
    iterations: 10,
    problem_size: 2147483648,
    run_time: 17.65,
    mops_total: 1216.95
  },{
    machine: 'xmt',
    version: 'xmt',
    problem: 'E',
    nnode: 24,
    ppn: 1,
    iterations: 10,
    problem_size: 2147483648,
    run_time: 20.44,
    mops_total: 1050.86
  },{
    machine: 'xmt',
    version: 'xmt',
    problem: 'E',
    nnode: 20,
    ppn: 1,
    iterations: 10,
    problem_size: 2147483648,
    run_time: 24.36,
    mops_total: 881.64
  },{
    machine: 'xmt',
    version: 'xmt',
    problem: 'E',
    nnode: 12,
    ppn: 1,
    iterations: 10,
    problem_size: 2147483648,
    run_time: 39.50,
    mops_total: 543.67
  },{
    machine: 'xmt',
    version: 'xmt',
    problem: 'E',
    nnode: 8,
    ppn: 1,
    iterations: 10,
    problem_size: 2147483648,
    run_time: 58.60,
    mops_total: 366.47
  },{
    machine: 'xmt',
    version: 'xmt',
    problem: 'E',
    nnode: 4,
    ppn: 1,
    iterations: 10,
    problem_size: 2147483648,
    run_time: 115.96,
    mops_total: 185.20
  },{
    machine: 'xmt',
    version: 'xmt',
    problem: 'E',
    nnode: 2,
    ppn: 1,
    iterations: 10,
    problem_size: 2147483648,
    run_time: 231.55,
    mops_total: 92.74
  }
]

require 'experiments'
db = Sequel.sqlite(File.expand_path "~/exp/sosp.db")
results.each do |r|
  g = prepare_table(:intsort, r, db)
  g.insert(r)
end
