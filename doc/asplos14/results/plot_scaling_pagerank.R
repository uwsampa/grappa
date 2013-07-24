#!/usr/bin/env Rscript
source("common.R")

# q2 <- "select * from pagerank"

d <- db("select * from pagerank", db="myers_pagerank.db",  c("periodic_poll_ticks", "ppn"))
# d <- db("select * from pagerank", db="sosp.db",  c("periodic_poll_ticks", "ppn"))
# d$mteps <- d$harmonic_mean_teps / 1e6
d$mteps <- d$actual_nnz / d$pagerank_time / 1e6
# d$capacity_flushes <- d$rdma_capacity_flushes
# d$fraction_flat <- d$cmp_swaps_shorted/(d$cmp_swaps_total)



# d.melt <- melt(d, measure=c("avg_mteps"))

d.sub <- subset(d,
  scale == 25 & 
  # nnz_factor <= 24 & nnz_factor >= 16 &
  ppn == 16 &
  # tag == 'pairdev' &
  nnz_factor == 16 &
  loop_threshold == 16 &
  num_starting_workers == 1024
)
d.sub$avg_mteps <- ddply(d.sub, .(nnz_factor), function(df) max(df$mteps))

# ddply(d.bfs.sub, .(tag), function(df) df[df$max_teps == max(df$max_teps),])



g <- ggplot(d.sub,
  # scale == 27 & nnode <= 64 
  # & (grepl('xmt_.*', version) |
  #   (grepl('mpi_.*', version) & ppn == 16 ) |
  #   (grepl('localized', version) & ppn == 16 & num_starting_workers == 1024) |
  #   (ppn == 16 & aggregator_autoflush_ticks > 1e6 )
  # )
  aes(
    x=nnode,
    y=mteps,
    color=x(nnz_factor),
    group=nnz_factor,
    # group=x(loop_threshold,tag,ppn,num_starting_workers,aggregator_autoflush_ticks),
    # label=round(avg_mteps),
  ))+
  xlab("Nodes")+ylab("MTEPS")+scale_x_continuous(breaks=c(0,8,16,32,48,64))+
  guides(color=FALSE)+
  # geom_point()+
  # geom_smooth(fill=NA,size=1.6,method=loess,enp.target=4)+
  # geom_line(size=1.6)+
  stat_summary(fun.y=mean, geom='line',size=1.6)+
  # geom_text(size=3, hjust=-0.4, vjust=0.3)+
  # facet_grid(variable~scale, scales="free", labeller=label_pretty)+
  expand_limits(y=0)+
  sosp_theme
g

ggsave(plot=g, filename="plot_scaling_pagerank.pdf", width=3.5, height=3.5)

