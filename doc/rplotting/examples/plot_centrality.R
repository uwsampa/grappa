#!/usr/bin/env Rscript
library(sqldf)
library(ggplot2)

# if you want to load external fonts (e.g. Humor Sans)
# library(extrafont)
# loadfonts()

source("../common.R")

db <- function(query) {
  d <- sqldf(query, dbname="sosp.db")
  
  # d$nnode <- factor(d$nnode) # make field categorical/discrete
  d$ppn   <- factor(d$ppn)
  d$scale <- factor(d$scale)
  return(d)
}

p <- ggplot(db("select * from centrality where scale >= 24"), 
  aes(x=nnode, y=centrality_teps, group=machine, color=ppn, shape=ppn))+
  ggtitle("Centrality")+xlab("Nodes")+ylab("TEPS")+
  sosp_theme+
  coord_cartesian(xlim=c(0,140))+ # viewing area
  # scale_x_continuous(breaks=seq(32, 128, 32))+ # axis ticks seq(from,to,by)
  scale_x_continuous(breaks=c(16,32,64,128))+    # list of axis labels to show
  geom_point()+ # show points
  stat_summary(fun.y="max", geom="line")+ # show line with max of each line
  facet_wrap(~scale) # make new plots for each scale
  
p # plot it
# ggsave("bfs_centrality.pdf", plot=p) # or save it

md <- melt(db("select * from centrality where scale >= 24"))

p <- ggplot(db("select * from centrality where scale >= 24"), 
  aes(x=nnode, y=centrality_teps, group=machine, color=ppn, shape=ppn))+
  ggtitle("Centrality")+xlab("Nodes")+ylab("Time")+
  sosp_theme+
  coord_cartesian(xlim=c(0,140))+ # viewing area
  # scale_x_continuous(breaks=seq(32, 128, 32))+ # axis ticks seq(from,to,by)
  scale_x_continuous(breaks=c(16,32,64,128))+    # list of axis labels to show
  geom_point()+ # show points
  stat_summary(fun.y="max", geom="line")+ # show line with max of each line
  facet_wrap(~scale) # make new plots for each scale
