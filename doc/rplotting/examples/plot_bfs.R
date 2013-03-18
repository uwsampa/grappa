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
  return(d)
}

p <- ggplot(db("select * from bfs where scale==30 or scale==26"), 
  aes(x=nnode, y=max_teps, group=mpibfs, color=mpibfs, shape=ppn))+
  ggtitle("BFS")+xlab("Nodes")+ylab("TEPS")+
  sosp_theme+
  geom_point()+ # show points
  # geom_line(mapping=aes(x=nnode,y=max_teps,group=ppn))+
  # stat_summary(fun.y="max", geom="line")+ # show line with max of each line
  facet_wrap(~scale) # make new plots for each scale
  
p # plot it
ggsave("plots/bfs_teps.pdf", plot=p) # or save it

# d <- db("select *,nnode*ppn as nproc from bfs where scale==30 or scale==26")
# p.nproc <- ggplot(d, aes(x=nproc, y=max_teps, group=mpibfs, color=mpibfs, shape=ppn))+
#   ggtitle("BFS")+xlab("Cores")+ylab("TEPS")+
#   scale_x_continuous(breaks=seq(0, 2048, 512))+ # axis ticks seq(from,to,by)
#   sosp_theme+
#   geom_point()+ # show points
#   # stat_summary(fun.y="max", geom="line")+ # show line with max of each line
#   facet_wrap(~scale) # make new plots for each scale
# p.nproc
# ggsave("plots/bfs_nproc.pdf", plot=p.nproc) # plot it
