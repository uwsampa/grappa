#!/usr/bin/env Rscript
setwd("~/dev/hub/grappa/doc/pgas13/data")
source("common.R")

q2 <- "select * from bfs where bfs_version like 'beamer%'"

d <- db(q2, c("nnode", "periodic_poll_ticks", "flat_combining"))
d$global_vector_push_ops_per_msg <- with(d, global_vector_push_ops/global_vector_push_msgs)

d$version_combining <- paste(d$bfs_version,":flat<",d$flat_combining,">",sep="")
d$mteps <- d$max_teps/1e6

d.melt <- melt(d, measure=c("global_vector_push_ops_per_msg", "mteps")) 

g <- ggplot(subset(d.melt,
    ppn == 16 & scale == 26
  ), aes(
    x=num_starting_workers,
    y=value,
    color=x(version_combining,ppn),
    # shape=aggregator_autoflush_ticks,
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
ggsave(plot=g, filename="plots/bfs_mess.pdf", width=10, height=8)

d$nw <- d$num_starting_workers

d$fc_version <- sapply(paste(d$bfs_version,d$flat_combining,sep='_'),switch,
  beamer_0='async', beamer_1='async', beamer_queue_0='none', beamer_queue_1='full'
)
# d$fc_version <- paste(d$bfs_version,d$flat_combining,sep='_')
# d$fc_version

g <- ggplot(subset(d,
    ppn == 16 & scale == 26
    & (num_starting_workers == 1024 & grepl('full|none', fc_version) 
      | num_starting_workers >= 256 & grepl('async', fc_version))
    & aggregator_autoflush_ticks > 100000
  ), aes(
    x=nnode,
    y=harmonic_mean_teps/1e6,
    color=fc_version,
    # shape=,
    linetype=fc_version,
    group=x(fc_version,num_starting_workers),
    label=x(num_starting_workers,aggregator_autoflush_ticks),
  ))+
  geom_point()+
  # geom_text(size=2,position="jitter")+
  geom_smooth(size=1, fill=NA)+
  xlab("Nodes")+
  scale_color_discrete(name="Flat Combining")+
  scale_linetype_discrete(name="Flat Combining")+
  # facet_grid(nw~., scales="free", labeller=label_pretty)+
  ylab("MTEPS")+expand_limits(y=0)+
  my_theme

ggsave(plot=g, filename="plots/bfs_perf.pdf", width=7, height=5)

