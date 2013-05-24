#!/usr/bin/env Rscript
library(sqldf)
library(ggplot2)
library(reshape)
library(extrafont)
loadfonts()
source("common.R")

q1 <- "select * from bfs where scale == 23 or scale == 26"
q2 <- "select * from bfs where scale == 26 and bfs_version like 'beamer_queue'"


d <- db(q2, c("nnode", "num_starting_workers", "aggregator_autoflush_ticks", "periodic_poll_ticks"))
d$global_vector_push_ops_per_msg <- with(d, global_vector_push_ops/global_vector_push_msgs)

d$version_combining <- paste(d$bfs_version,":flat<",d$flat_combining,">",sep="")

d <- melt(d, measure=c("global_vector_push_ops_per_msg", "max_teps"))

g <- ggplot(d, aes(
    x=num_starting_workers,
    y=value,
    color=version_combining,
    shape=aggregator_autoflush_ticks,
    group=flat_combining
    # label=nnode~ppn,
  ))+
  geom_point()+
  geom_smooth(aes(linetype=flat_combining))+
  # facet_grid(~variable~ppn, scales="free", labeller=label_bquote(.(prettify(x))))+
  facet_grid(~variable~nnode~ppn~scale, scales="free", labeller=label_pretty)+
  ylab("")+
  expand_limits(y=0)+
  my_theme

ggsave(plot=g, filename="plots/bfs_perf.pdf", scale=1.2)

