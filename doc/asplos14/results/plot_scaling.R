#!/usr/bin/env Rscript
source("common.R")

d <- db("select * from bfs", db="sosp.db",  c("periodic_poll_ticks", "ppn"))
# d$mteps <- d$harmonic_mean_teps / 1e6
d$mteps <- d$max_teps / 1e6
# d$capacity_flushes <- d$rdma_capacity_flushes
# d$fraction_flat <- d$cmp_swaps_shorted/(d$cmp_swaps_total)
d$fnnode <- factor(d$nnode)
# d.melt <- melt(d, measure=c("mteps"))

cmp <- subset(d,
  scale == 27 & nnode != 128 & (
    grepl('xmt_.*', bfs_version) |
    (grepl('mpi_.*', bfs_version) & ppn == 16 ) |
    (grepl('localized', bfs_version) & ppn == 16 & num_starting_workers == 1024) |
    (ppn == 16 & aggregator_autoflush_ticks > 1e6 )
))
grappa_only <- subset(d,
  scale == 27
  & nnode != 128
  & grepl('grappa_adj', bfs_version)
  & shared_pool_max < 2**15
  # & aggregator_autoflush_ticks == 1e6
  # & shared_pool_max <= 2**14 
  # & shared_pool_size == 2**16
)

g <- ggplot(grappa_only, aes(
    x=nnode,
    y=mteps,
    # color=x(bfs_version,shared_pool_max,shared_pool_size,aggregator_autoflush_ticks),
    color=bfs_version,
    linetype=ppn,
    shape=ppn,
    group=bfs_version,
    # group=x(bfs_version,ppn,num_starting_workers,aggregator_autoflush_ticks, shared_pool_max,shared_pool_size),
    # label=round(value),
  ))+
  xlab("Nodes")+scale_x_continuous(breaks=c(0,8,16,32,48,64))+
  ylab("MTEPS")+
  guides(color=FALSE,linetype=FALSE,shape=FALSE)+
  # geom_point()+
  # geom_line()+
  # geom_smooth(fill=NA,size=1.6)+
  stat_summary(fun.y=mean, geom='line',size=1.6)+
  # geom_text(size=3, hjust=-0.4, vjust=0.3)+
  # facet_grid(variable~scale, scales="free", labeller=label_pretty)+
  expand_limits(y=0)+
  sosp_theme
g
ggsave(plot=g, filename="plot.scaling.bfs.pdf", width=3.5, height=3.5)
