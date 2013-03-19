#!/usr/bin/env Rscript
library(sqldf)
library(ggplot2)

# if you want to load external fonts (e.g. Humor Sans)
# library(extrafont)
# loadfonts()

source("../common.R")

db <- function(query) {
  d <- sqldf(query, dbname="sosp.db")
  
  d$nnode <- factor(d$nnode) # make field categorical
  d$ppn   <- factor(d$ppn)
  d$scale <- factor(d$scale)
  d$num_starting_workers <- factor(d$num_starting_workers)
  return(d)
}

p <- ggplot(db("select * from bfs where bfs_version is 'localized'"), 
  aes(group=ppn, color=ppn, shape=ppn))+
  ggtitle("BFS: Grappa")+
  sosp_theme+
  # geom_point()+ # show points
  # geom_line(mapping=aes(x=nnode,y=max_teps,group=ppn))+
  # stat_summary(fun.y="max", geom="line")+ # show line with max of each line
  facet_wrap(~scale) # make new plots for each scale

p.nn <- p+geom_point(mapping=aes(x=nnode,y=harmonic_mean_teps))
p.nn
# ggsave("plots/bfs_grappa.pdf", plot=p) # or save it

p.nw <- p+geom_point(mapping=aes(x=num_starting_workers,y=harmonic_mean_teps),
  data=db("select * from bfs where
      bfs_version is 'localized' 
      and ppn == 4
      and aggregator_autoflush_ticks == 3000000
      and periodic_poll_ticks == 20000
      and loop_threshold == 16
  "))
p.nw

p.poll <- p+
  geom_point(mapping=aes(x=periodic_poll_ticks,y=harmonic_mean_teps),
    data=db("select * from bfs where
        bfs_version is 'localized'
  "))
p.poll


# d <- db("select *,nnode*ppn as nproc from bfs where scale==30 or scale==26")
# p.nproc <- ggplot(d, aes(x=nproc, y=max_teps, group=ppn, color=ppn, shape=ppn))+
#   ggtitle("BFS")+xlab("Cores")+ylab("TEPS")+
#   scale_x_continuous(breaks=seq(0, 2048, 512))+ # axis ticks seq(from,to,by)
#   sosp_theme+
#   geom_point()+ # show points
#   # stat_summary(fun.y="max", geom="line")+ # show line with max of each line
#   facet_wrap(~scale) # make new plots for each scale
# p.nproc
# ggsave("plots/bfs_nproc.pdf", plot=p.nproc) # plot it
