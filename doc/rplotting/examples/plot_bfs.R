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
  stat_summary(fun.y="max", geom="line")+ # show line with max of each line
  facet_wrap(~scale) # make new plots for each scale
  
p # plot it
# ggsave("bfs_teps.pdf", plot=p) # or save it

