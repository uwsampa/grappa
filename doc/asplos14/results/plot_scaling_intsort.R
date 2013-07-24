#!/usr/bin/env Rscript
source("common.R")

q2 <- "select * from intsort"

d <- db(q2, db="sosp.db",  c("periodic_poll_ticks"))
# d$mteps <- d$harmonic_mean_teps / 1e6
# d$capacity_flushes <- d$rdma_capacity_flushes
# d$fraction_flat <- d$cmp_swaps_shorted/(d$cmp_swaps_total)

d.melt <- melt(d, measure=c("mops_total"))

g <- ggplot(subset(d.melt,
    ((problem == 'E' & version == 'grappa') | (problem == 'D' & tag == 'mpi'))
    & ppn == 16
    & aggregator_autoflush_ticks == 3e6
  ), aes(
    x=nnode,
    y=value,
    color=x(version,shared_pool_max,shared_pool_size,grappa_version,aggregator_autoflush_ticks),
    linetype=x(version,ppn),
    shape=x(version,ppn),
    group=x(version, ppn, num_starting_workers, aggregator_autoflush_ticks, shared_pool_size, shared_pool_max, grappa_version),
    # label=x(round(value),num_starting_workers,aggregator_autoflush_ticks,shared_pool_max),
    label=value
  ))+
  xlab("Nodes")+ylab("Millions of Ops/s")+scale_x_continuous(breaks=c(0,8,16,32,48,64))+
  guides(color=FALSE,linetype=FALSE,shape=FALSE)+
  # geom_point()+
  geom_line(size=1.6)+
  # geom_text(size=3, hjust=-0.2, vjust=0.3)+
  # facet_grid(variable~., scales="free", labeller=label_pretty)+
  expand_limits(y=0)+
  sosp_theme
g

ggsave(plot=g, filename="plot.scaling.intsort.pdf", width=3.5, height=3.5)
