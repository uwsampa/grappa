#!/usr/bin/env Rscript
source("common.R")

d <- db("select * from uts", db="sosp_uts.db",  c("periodic_poll_ticks", "ppn"))
# d$mteps <- d$harmonic_mean_teps / 1e6
d$perf <- d$uts_num_searched_nodes / d$search_runtime / 1e6
# d$capacity_flushes <- d$rdma_capacity_flushes
# d$fraction_flat <- d$cmp_swaps_shorted/(d$cmp_swaps_total)

d$latency <- with(d, DelegateStats_average_latency / GrappaStats_tick_rate * 1e3)
d$idle_percent <- with(d, rdma_idle_flushes / SchedulerStats_scheduler_count * 100)

d.melt <- melt(d, measure=c("perf", "latency", "idle_percent"))
levels(d.melt$variable) <- c('Throughput\n(MVerts/s)', 'Avg delegate\n latency (ms)', '% Idle')

g <- ggplot(subset(d.melt,
  tree == 'T1XL' & nnode == 48
  # & ppn == 16
  & aggregator_autoflush_ticks == 1e6
  ),aes(
    x=num_starting_workers,
    y=value,
    color=x(aggregator_autoflush_ticks),
    linetype=ppn,
    shape=ppn,
    group=x(ppn,aggregator_autoflush_ticks),
    label=round(value),
  ))+
  xlab("Number of workers")+scale_x_continuous(breaks=c(0,256,512,768,1024))+
  guides(fill=FALSE,color=FALSE,shape=FALSE,linetype=FALSE)+
  geom_point()+
  geom_line()+
  # geom_text(size=3, hjust=-0.4, vjust=0.3)+
  facet_grid(variable~., scales="free", labeller=label_value)+
  ylab("")+
  expand_limits(y=0)+
  sosp_theme
g

ggsave(plot=g, filename="plot_concurrency.pdf", width=3, height=5)
