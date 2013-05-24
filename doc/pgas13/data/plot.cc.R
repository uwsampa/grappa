#!/usr/bin/env Rscript
library(sqldf)
library(ggplot2)
library(reshape)
library(extrafont)
loadfonts()
source("common.R")

d <- db(
  "select * from cc
   where scale == 26
   ",
  c("nnode", "ppn",
    "num_starting_workers",
    "aggregator_autoflush_ticks",
    "periodic_poll_ticks",
    "flat_combining",
    "cc_concurrent_roots"
  )
)

d$ops_per_msg <- with(d, hashset_insert_ops/hashset_insert_msgs)
d$throughput <- with(d, (2**scale*16)/components_time)
d$roots <- d$cc_concurrent_roots

d$fc_version <- sapply(paste('v',d$flat_combining,d$cc_insert_async,sep=''),switch,
  v0NA='none', v1NA='fc', v11='async', v10='fc', v00='none'
)

g <- ggplot(subset(d, cc_hash_size == 16384), aes(
    x=num_starting_workers,
    y=throughput,
    color=roots,
    shape=fc_version,
    # label=trial,
    group=x(cc_concurrent_roots,fc_version)
  ))+
  geom_point()+
  # geom_smooth(aes(linetype=fc_version),stat=stat_smooth(se=FALSE))+
  # geom_smooth(aes(linetype=fc_version),fill=NA)+
  geom_line(aes(linetype=fc_version))+
  # geom_text(size=2,hjust=-0.2,vjust=1)+
  facet_grid(~nnode~ppn~scale~cc_hash_size, scales="free", labeller=label_pretty)+
  ylab("")+
  expand_limits(y=0)+
  my_theme
ggsave(plot=g, filename="plots/cc_perf.pdf", scale=1.4)

# g <- ggplot(d.m, aes(
#     x=cc_concurrent_roots,
#     y=value,
#     color=fc_version, fill=fc_version,
#     shape=fc_version,
#     label=trial,
#     group=paste(cc_concurrent_roots,fc_version)
#   ))+
#   geom_bar(stat="identity", position="dodge")+
#   # geom_point()+
#   # geom_line(aes(linetype=fc_version))+
#   geom_text(size=2,hjust=-0.2,vjust=1)+
#   facet_grid(~variable~nnode~ppn~scale, scales="free", labeller=label_pretty)+
#   ylab("")+
#   expand_limits(y=0)+
#   my_theme
# ggsave(plot=g, filename="cc_perf_clean.pdf", scale=1.4)


d.t <- melt(d, measure=c("cc_propagate_time","cc_reduced_graph_time","cc_set_insert_time"))
d.t$cc_time_names <- d.t$variable
d.t$cc_time_values <- d.t$value

g.t <- ggplot(d.t,
  aes(
    x=cc_concurrent_roots,
    y=cc_time_values,
    color=cc_time_names, fill=cc_time_names
  ))+
  facet_grid(~scale~ppn~nnode~cc_hash_size~fc_version, labeller=label_pretty)+
  geom_bar(stat="identity")+
  my_theme
ggsave(plot=g.t, filename="plots/cc_times.pdf", scale=1.4)
