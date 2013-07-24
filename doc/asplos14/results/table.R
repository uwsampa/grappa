library(sqldf)
library(plyr)

###################
# bfs
d.bfs <- sqldf("select * from bfs", dbname="sosp.db")
d.bfs.sub <- subset(d.bfs, scale == 27 & nnode == 64 )
d.bfs.best <- ddply(d.bfs.sub, .(bfs_version), function(df) df[df$max_teps == max(df$max_teps),])
d.bfs.best$relative_teps <- ddply(d.bfs.best, .(bfs_version), function(df) df$max_teps/d.bfs.best[d.bfs.best$bfs_version == 'grappa_adj','max_teps'])

r.bfs <- subset(d.bfs.best, select=c('id','bfs_version','max_teps','relative_teps'), grepl('xmt_manhattan|mpi_simple|grappa_adj', bfs_version))

###################
# intsort
d.is <- sqldf("select * from intsort", dbname="sosp.db")
d.is.sub <- subset(d.is, version == 'grappa' & problem == 'E' & ppn == 16)
r.is <- d.is.sub[d.is.sub$mops_total == max(d.is.sub$mops_total),]

d.is.mpi <- subset(d.is, problem == 'D' & nnode == 64 & ppn == 16, select=c('maxtime','alltoall_time_mean', 'local_bucket_time_mean','scatter_time'))
# r.is.mpi <- d.is.mpi[d.is.mpi$mops_total == max(d.is.mpi$mops_total),]
r.is.mpi <- subset(d.is, problem == 'D' & nnode == 64 & ppn == 16 & maxtime > 0)

cmp.intsort <- r.is.mpi$mops_total / r.is$mops_total
