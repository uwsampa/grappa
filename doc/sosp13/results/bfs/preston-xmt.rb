#!/usr/bin/env ruby
require "experiments"

db = Sequel.sqlite("sosp.db")
table = :bfs

data = [
  { bfs_version: "xmt_decomposition", tag: "cougar#decomposition", machine: "cougarxmt", scale: 23, nnode:  16, mteps:  69 },
  { bfs_version: "xmt_decomposition", tag: "cougar#decomposition", machine: "cougarxmt", scale: 23, nnode:  32, mteps: 116 },
  { bfs_version: "xmt_decomposition", tag: "cougar#decomposition", machine: "cougarxmt", scale: 23, nnode:  48, mteps: 152 },
  { bfs_version: "xmt_decomposition", tag: "cougar#decomposition", machine: "cougarxmt", scale: 23, nnode:  64, mteps: 177 },
  { bfs_version: "xmt_decomposition", tag: "cougar#decomposition", machine: "cougarxmt", scale: 23, nnode:  80, mteps: 186 },
  { bfs_version: "xmt_decomposition", tag: "cougar#decomposition", machine: "cougarxmt", scale: 23, nnode:  96, mteps: 206 },
  { bfs_version: "xmt_decomposition", tag: "cougar#decomposition", machine: "cougarxmt", scale: 23, nnode: 112, mteps: 204 },
  { bfs_version: "xmt_decomposition", tag: "cougar#decomposition", machine: "cougarxmt", scale: 23, nnode: 128, mteps: 216 },
  
  { bfs_version: "xmt_decomposition", tag: "cougar#decomposition", machine: "cougarxmt", scale: 27, nnode:  16, mteps:  82 },
  { bfs_version: "xmt_decomposition", tag: "cougar#decomposition", machine: "cougarxmt", scale: 27, nnode:  32, mteps: 156 },
  { bfs_version: "xmt_decomposition", tag: "cougar#decomposition", machine: "cougarxmt", scale: 27, nnode:  48, mteps: 221 },
  { bfs_version: "xmt_decomposition", tag: "cougar#decomposition", machine: "cougarxmt", scale: 27, nnode:  64, mteps: 280 },
  { bfs_version: "xmt_decomposition", tag: "cougar#decomposition", machine: "cougarxmt", scale: 27, nnode:  80, mteps: 330 },
  { bfs_version: "xmt_decomposition", tag: "cougar#decomposition", machine: "cougarxmt", scale: 27, nnode:  96, mteps: 373 },
  { bfs_version: "xmt_decomposition", tag: "cougar#decomposition", machine: "cougarxmt", scale: 27, nnode: 112, mteps: 312 },
  { bfs_version: "xmt_decomposition", tag: "cougar#decomposition", machine: "cougarxmt", scale: 27, nnode: 128, mteps: 389 },

  { bfs_version: "xmt_manhattan", tag: "cougar#manhattan",     machine: "cougarxmt", scale: 23, nnode:  16, mteps: 177 },
  { bfs_version: "xmt_manhattan", tag: "cougar#manhattan",     machine: "cougarxmt", scale: 23, nnode:  32, mteps: 289 },
  { bfs_version: "xmt_manhattan", tag: "cougar#manhattan",     machine: "cougarxmt", scale: 23, nnode:  48, mteps: 368 },
  { bfs_version: "xmt_manhattan", tag: "cougar#manhattan",     machine: "cougarxmt", scale: 23, nnode:  64, mteps: 402 },
  { bfs_version: "xmt_manhattan", tag: "cougar#manhattan",     machine: "cougarxmt", scale: 23, nnode:  80, mteps: 427 },
  { bfs_version: "xmt_manhattan", tag: "cougar#manhattan",     machine: "cougarxmt", scale: 23, nnode:  96, mteps: 435 },
  { bfs_version: "xmt_manhattan", tag: "cougar#manhattan",     machine: "cougarxmt", scale: 23, nnode: 112, mteps: 440 },
  { bfs_version: "xmt_manhattan", tag: "cougar#manhattan",     machine: "cougarxmt", scale: 23, nnode: 128, mteps: 441 },
  
  { bfs_version: "xmt_manhattan", tag: "cougar#manhattan",     machine: "cougarxmt", scale: 27, nnode:  16, mteps: 186 },
  { bfs_version: "xmt_manhattan", tag: "cougar#manhattan",     machine: "cougarxmt", scale: 27, nnode:  32, mteps: 326 },
  { bfs_version: "xmt_manhattan", tag: "cougar#manhattan",     machine: "cougarxmt", scale: 27, nnode:  48, mteps: 425 },
  { bfs_version: "xmt_manhattan", tag: "cougar#manhattan",     machine: "cougarxmt", scale: 27, nnode:  64, mteps: 505 },
  { bfs_version: "xmt_manhattan", tag: "cougar#manhattan",     machine: "cougarxmt", scale: 27, nnode:  80, mteps: 570 },
  { bfs_version: "xmt_manhattan", tag: "cougar#manhattan",     machine: "cougarxmt", scale: 27, nnode:  96, mteps: 606 },
  { bfs_version: "xmt_manhattan", tag: "cougar#manhattan",     machine: "cougarxmt", scale: 27, nnode: 112, mteps: 645 },
  { bfs_version: "xmt_manhattan", tag: "cougar#manhattan",     machine: "cougarxmt", scale: 27, nnode: 128, mteps: 674 },

  { bfs_version: "xmt_manhattan", tag: "cougar#small", machine: "cougarxmt", scale: 25, nnode: 2, mteps: 8.67 },
  { bfs_version: "xmt_manhattan", tag: "cougar#small", machine: "cougarxmt", scale: 25, nnode: 3, mteps: 13.3 },
  { bfs_version: "xmt_manhattan", tag: "cougar#small", machine: "cougarxmt", scale: 25, nnode: 4, mteps: 17.8 },
  { bfs_version: "xmt_manhattan", tag: "cougar#small", machine: "cougarxmt", scale: 25, nnode: 5, mteps: 22.1 },
  { bfs_version: "xmt_manhattan", tag: "cougar#small", machine: "cougarxmt", scale: 25, nnode: 6, mteps: 26.5 },
  { bfs_version: "xmt_manhattan", tag: "cougar#small", machine: "cougarxmt", scale: 25, nnode: 7, mteps: 30.7 },
  { bfs_version: "xmt_manhattan", tag: "cougar#small", machine: "cougarxmt", scale: 25, nnode: 8, mteps: 35.0 },
  { bfs_version: "xmt_manhattan", tag: "cougar#small", machine: "cougarxmt", scale: 25, nnode: 9, mteps: 39.2 },
  { bfs_version: "xmt_manhattan", tag: "cougar#small", machine: "cougarxmt", scale: 25, nnode: 10, mteps: 43.4 },
  { bfs_version: "xmt_manhattan", tag: "cougar#small", machine: "cougarxmt", scale: 25, nnode: 11, mteps: 47.5 },
  { bfs_version: "xmt_manhattan", tag: "cougar#small", machine: "cougarxmt", scale: 25, nnode: 12, mteps: 51.7 },
  { bfs_version: "xmt_manhattan", tag: "cougar#small", machine: "cougarxmt", scale: 25, nnode: 13, mteps: 55.6 },
  { bfs_version: "xmt_manhattan", tag: "cougar#small", machine: "cougarxmt", scale: 25, nnode: 14, mteps: 60.0 },
  { bfs_version: "xmt_manhattan", tag: "cougar#small", machine: "cougarxmt", scale: 25, nnode: 15, mteps: 63.9 },
  { bfs_version: "xmt_manhattan", tag: "cougar#small", machine: "cougarxmt", scale: 25, nnode: 16, mteps: 67.6 },

  { bfs_version: "xmt_decomposition", tag: "cougar#small", machine: "cougarxmt", scale: 23, nnode: 2, mteps: 8.77 },
  { bfs_version: "xmt_decomposition", tag: "cougar#small", machine: "cougarxmt", scale: 23, nnode: 3, mteps: 12.9 },
  { bfs_version: "xmt_decomposition", tag: "cougar#small", machine: "cougarxmt", scale: 23, nnode: 4, mteps: 17.2 },
  { bfs_version: "xmt_decomposition", tag: "cougar#small", machine: "cougarxmt", scale: 23, nnode: 5, mteps: 21.1 },
  { bfs_version: "xmt_decomposition", tag: "cougar#small", machine: "cougarxmt", scale: 23, nnode: 6, mteps: 25.2 },
  { bfs_version: "xmt_decomposition", tag: "cougar#small", machine: "cougarxmt", scale: 23, nnode: 7, mteps: 29.2 },
  { bfs_version: "xmt_decomposition", tag: "cougar#small", machine: "cougarxmt", scale: 23, nnode: 8, mteps: 33.1 },
  { bfs_version: "xmt_decomposition", tag: "cougar#small", machine: "cougarxmt", scale: 23, nnode: 9, mteps: 36.9 },
  { bfs_version: "xmt_decomposition", tag: "cougar#small", machine: "cougarxmt", scale: 23, nnode: 10, mteps: 40.5 },
  { bfs_version: "xmt_decomposition", tag: "cougar#small", machine: "cougarxmt", scale: 23, nnode: 11, mteps: 44.3 },
  { bfs_version: "xmt_decomposition", tag: "cougar#small", machine: "cougarxmt", scale: 23, nnode: 12, mteps: 47.9 },
  { bfs_version: "xmt_decomposition", tag: "cougar#small", machine: "cougarxmt", scale: 23, nnode: 13, mteps: 51.4 },
  { bfs_version: "xmt_decomposition", tag: "cougar#small", machine: "cougarxmt", scale: 23, nnode: 14, mteps: 55.0 },
  { bfs_version: "xmt_decomposition", tag: "cougar#small", machine: "cougarxmt", scale: 23, nnode: 15, mteps: 58.7 },
  { bfs_version: "xmt_decomposition", tag: "cougar#small", machine: "cougarxmt", scale: 23, nnode: 16, mteps: 61.5  },
  
  { bfs_version: "xmt_decomposition", tag: "cougar#small", machine: "cougarxmt", scale: 25, nnode: 2, mteps: 8.97 },
  { bfs_version: "xmt_decomposition", tag: "cougar#small", machine: "cougarxmt", scale: 25, nnode: 4, mteps: 17.7 },
  { bfs_version: "xmt_decomposition", tag: "cougar#small", machine: "cougarxmt", scale: 25, nnode: 8, mteps: 35.1 },
  { bfs_version: "xmt_decomposition", tag: "cougar#small", machine: "cougarxmt", scale: 25, nnode: 16, mteps: 67.5 },
  
  { bfs_version: "xmt_decomposition", tag: "cougar#small", machine: "cougarxmt", scale: 25, nnode: 2, mteps: 8.96 },
  { bfs_version: "xmt_decomposition", tag: "cougar#small", machine: "cougarxmt", scale: 25, nnode: 4, mteps: 17.7 },
  { bfs_version: "xmt_decomposition", tag: "cougar#small", machine: "cougarxmt", scale: 25, nnode: 8, mteps: 35.0 },
  { bfs_version: "xmt_decomposition", tag: "cougar#small", machine: "cougarxmt", scale: 25, nnode: 16, mteps: 67.6 },
]

data.each do |r|
  teps = 1e6 * r[:mteps].to_f
  r.delete(:mteps)
  r[:max_teps] = teps
  r[:min_teps] = teps
  r[:harmonic_mean_teps] = teps
  r[:harmonic_stddev_teps] = 0
  r[:ppn] = 1
  r[:nproc] = r[:nnode]
  r[:nbfs] = 4
  g = prepare_table(table, r, db)
  g.insert(r)
end

