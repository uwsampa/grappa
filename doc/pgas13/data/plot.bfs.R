#!/usr/bin/env Rscript
setwd("~/dev/hub/grappa/doc/pgas13/data")
source("common.R")

q2 <- "select * from bfs where bfs_version like 'beamer%'"

d <- db(q2, c("nnode", "num_starting_workers", "aggregator_autoflush_ticks", "periodic_poll_ticks"))
d$global_vector_push_ops_per_msg <- with(d, global_vector_push_ops/global_vector_push_msgs)

d$version_combining <- paste(d$bfs_version,":flat<",d$flat_combining,">",sep="")
d$mteps <- d$max_teps/1e6

d <- melt(d, measure=c("global_vector_push_ops_per_msg", "mteps")) 

g <- ggplot(subset(d,
    ppn == 16
  ), aes(
    x=num_starting_workers,
    y=value,
    color=x(version_combining,ppn),
    shape=aggregator_autoflush_ticks,
    group=x(version_combining,ppn)
    # label=nnode~ppn,
  ))+
  geom_point()+
  geom_smooth(aes(linetype=flat_combining))+
  # facet_grid(~variable~ppn, scales="free", labeller=label_bquote(.(prettify(x))))+
  facet_grid(~variable~scale~nnode, scales="free", labeller=label_pretty)+
  ylab("")+
  expand_limits(y=0)+
  my_theme

ggsave(plot=g, filename="plots/bfs_mess.pdf", scale=1.2)

g <- ggplot(subset(d,
    ppn == 16 & scale == 26 & num_starting_workers == 1024
  ), aes(
    x=nnode,
    y=value,
    color=x(version_combining,ppn),
    shape=aggregator_autoflush_ticks,
    group=x(version_combining,ppn)
    # label=nnode~ppn,
  ))+
  geom_point()+
  geom_smooth(aes(linetype=flat_combining))+
  # facet_grid(~variable~ppn, scales="free", labeller=label_bquote(.(prettify(x))))+
  facet_grid(~variable~scale~nnode, scales="free", labeller=label_pretty)+
  ylab("")+
  expand_limits(y=0)+
  my_theme

ggsave(plot=g, filename="plots/bfs_perf.pdf", scale=1.2)

