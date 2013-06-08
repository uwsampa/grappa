#!/usr/bin/env Rscript
setwd("~/dev/hub/grappa/doc/pgas13/data")
source("common.R")

d <- db(
  "select * from cc
   ",
  c("nnode", "ppn",
    "num_starting_workers",
    "periodic_poll_ticks",
    "flat_combining",
    "cc_concurrent_roots"
  )
)

d$ops_per_msg <- with(d, hashset_insert_ops/hashset_insert_msgs)
d$mteps <- with(d, (2**scale*16)/components_time/1e6)
d$nroots <- d$cc_concurrent_roots
d$gce_flats <- with(d, gce_total_remote_completions/gce_completions_sent)

d$fc_version <- sapply(paste('v',d$flat_combining,d$cc_insert_async,sep=''),switch,
  v0NA='none', v1NA='distributed', v11='custom', v10='distributed', v00='none', '??'
)

g <- ggplot(subset(d, cc_hash_size <= 16384 & scale == 26 & ppn == 16
    & num_starting_workers == 2048
    & aggregator_autoflush_ticks == 5e5
  ), aes(
    x=nnode,
    y=mteps,
    color=x(nroots,version,fc_version),
    shape=fc_version,
    label=x(aggregator_autoflush_ticks/1e6,num_starting_workers),
    group=x(nroots,version,fc_version,aggregator_autoflush_ticks),
    linetype=fc_version,
  ))+
  geom_point()+
  # geom_smooth(aes(linetype=fc_version),stat=stat_smooth(se=FALSE))+
  # geom_smooth(aes(linetype=fc_version),fill=NA)+
  geom_line()+
  geom_text(size=2,hjust=-0.2,vjust=1)+
  facet_grid(~cc_hash_size~ppn~scale~nnode, scales="free", labeller=label_pretty)+
  # ylab("")+
  expand_limits(y=0)+
  theme(strip.text=element_text(size=rel(0.4)),
        axis.text.x=element_text(size=rel(0.75)))+
  my_theme
ggsave(plot=g, filename="plots/cc_mess.pdf", scale=1.4)

g <- ggplot(subset(d, 
    cc_hash_size == 16384 & scale == 26 & ppn == 16
    & ( fc_version == 'custom' & num_starting_workers == 2048 & aggregator_autoflush_ticks == 5e5
      | fc_version == 'distributed' & num_starting_workers == 2048 & aggregator_autoflush_ticks == 5e5
      | fc_version == 'none' & num_starting_workers == 2048 & aggregator_autoflush_ticks == 1e5
    )
    & cc_concurrent_roots == 2048
    # & version == 'NA'
    # & aggregator_autoflush_ticks >= 500000
  ), aes(
    x=nnode,
    y=mteps,
    color=fc_version,
    shape=fc_version,
    linetype=fc_version,
    group=x(cc_concurrent_roots,fc_version),
    label=x(aggregator_autoflush_ticks/1e6,num_starting_workers),
  ))+
  # geom_point()+
  geom_smooth(size=1, fill=NA)+
  # geom_text(size=2,hjust=-0.2,vjust=1)+
  # geom_line()+
  xlab("Nodes")+
  scale_color_discrete(name="Flat Combining")+
  scale_linetype_discrete(name="Flat Combining")+
  # facet_grid(scales="free", labeller=label_pretty)+
  ylab("MTEPS")+expand_limits(y=0)+
  my_theme
ggsave(plot=g, filename="plots/cc_perf.pdf", width=7, height=5)

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

g.t <- ggplot(subset(d.t, cc_hash_size == 16384 & ppn == 16),
  aes(
    x=cc_concurrent_roots,
    y=cc_time_values,
    color=cc_time_names, fill=cc_time_names
  ))+
  facet_grid(scale+cc_hash_size+fc_version~nnode+ppn, labeller=label_pretty)+
  geom_bar(stat="identity")+
  my_theme
# ggsave(plot=g.t, filename="plots/cc_times.pdf", width=16, height=10)

d.stat <- melt(d, measure=c('mteps', 'ce_remote_completions', 'cc_set_size', 'gce_flats'))
g.stat <- ggplot(subset(d.stat, scale == 26 & fc_version != 'custom'), aes(
    x=num_starting_workers,
    y=value,
    color=nroots, fill=nroots,
    shape=ppn,
    linetype=fc_version,
    group=x(fc_version,nroots,ppn)
  ))+
  geom_point()+
  geom_line()+
  # geom_smooth(fill=NA)+
  facet_grid(~scale~variable~nnode, labeller=label_pretty, scales="free")+
  ylab("")+expand_limits(y=0)+my_theme
# ggsave(plot=g.stat, filename="plots/cc_stats.pdf", scale=1.5)





