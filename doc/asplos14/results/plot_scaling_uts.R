#!/usr/bin/env Rscript
source("common.R")

# q2 <- "select * from pagerank"

d <- db("select * from uts", db="sosp_uts.db",  c("periodic_poll_ticks", "ppn"))
d$perf <- d$uts_num_searched_nodes / d$search_runtime / 1e6

d.sub <- subset(d,
  tree == 'T1XL' &
  # scale == 25 & 
  # nnz_factor <= 24 & nnz_factor >= 16 &
  ppn == 16
  & aggregator_autoflush_ticks == 1e6
  # tag == 'pairdev' &
  # nnz_factor == 16 &
  & num_starting_workers == 512
)
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
    y=perf,
    color=x(loop_threshold,tag,aggregator_autoflush_ticks,num_starting_workers,periodic_poll_ticks),
    group=x(loop_threshold,tag,ppn,num_starting_workers,aggregator_autoflush_ticks),
    label=round(perf),
  ))+
  xlab("Nodes")+ylab("MVerts/s")+scale_x_continuous(breaks=c(0,8,16,32,48,64))+
  guides(color=FALSE)+
  # geom_point()+
  # geom_smooth(fill=NA)+
  geom_line(size=1.6)+
  # geom_text(size=3, hjust=-0.4, vjust=0.3)+
  # facet_grid(variable~scale, scales="free", labeller=label_pretty)+
  expand_limits(y=0)+
  sosp_theme
g

ggsave(plot=g, filename="plot.scaling.uts.pdf", width=3.5, height=3.5)
