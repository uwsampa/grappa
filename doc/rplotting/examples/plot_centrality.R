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

p <- ggplot(db("select *, centrality_teps/1e6 as MTEPS from centrality where scale >= 24"), 
  aes(x=nnode, y=MTEPS, group=machine, shape=ppn))+
  ggtitle("Centrality")+xlab("Nodes")+
  sosp_theme+
  coord_cartesian(xlim=c(0,140))+ # viewing area
  # scale_x_continuous(breaks=seq(32, 128, 32))+ # axis ticks seq(from,to,by)
  scale_x_continuous(breaks=c(16,32,64,128))+    # list of axis labels to show
  geom_point()+ # show points
  # stat_summary(fun.y="max", geom="line")+ # show line with max of each line
  facet_wrap(~scale) # make new plots for each scale
  
p # plot it
ggsave("plots/centrality_teps.pdf", plot=p) # or save it

# md <- melt(db("select * from centrality where scale >= 24"))

d <- db("select scale,nnode,ppn,centrality_teps,nnode*ppn as nproc from centrality where scale >= 24")
d$nproc <- as.numeric(d$nproc)
d$nnode <- factor(d$nnode)

p <- ggplot(d, aes(x=nproc, y=centrality_teps, color=nnode, shape=ppn))+
  ggtitle("Centrality")+xlab("Cores")+ylab("TEPS")+
  sosp_theme+
  coord_cartesian(xlim=c(0,1100))+ # viewing area
  scale_x_continuous(breaks=seq(0, 1024, 256))+ # axis ticks seq(from,to,by)
  # scale_x_continuous(breaks=c(16,32,64,128))+    # list of axis labels to show
  geom_point()+ # show points
  # stat_summary(fun.y="max", geom="line")+ # show line with max of each line
  facet_wrap(~scale) # make new plots for each scale
p