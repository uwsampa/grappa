#!/usr/bin/env Rscript
source("common.R")

# q2 <- "select * from pagerank"

d <- db("select * from uts", db="sosp_uts.db",  c("periodic_poll_ticks", "ppn"))
d$perf <- d$uts_num_searched_nodes / d$search_runtime / 1e6

d.sub <- subset(d,
  grepl('T1XL|T3L', tree) &
  ((tree == 'T1XL' & aggregator_autoflush_ticks == 1e6)
  | (tree == 'T3L' & aggregator_autoflush_ticks == 1e5))
  # scale == 25 & 
  # nnz_factor <= 24 & nnz_factor >= 16 &
  & ppn == 16
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
    color=tree,
    group=x(tree,aggregator_autoflush_ticks),
    label=round(perf),
  ))+
  xlab("Nodes")+ylab("MVerts/s")+scale_x_continuous(breaks=c(0,8,16,32,48,64))+
  # guides(color=FALSE)+
  scale_color_discrete(name="Tree:")+
  # geom_point()+
  # geom_smooth(fill=NA)+
  geom_line(size=1.4)+
  # geom_text(size=3, hjust=-0.4, vjust=0.3)+
  # facet_grid(variable~scale, scales="free", labeller=label_pretty)+
  expand_limits(y=0)+
  # theme(legend.position="bottom")+
  theme(legend.position=c(0.78,0.41))+
  sosp_theme
g

ggsave(plot=g, filename="plot_scaling_uts.pdf", width=4.25, height=4.25)
I