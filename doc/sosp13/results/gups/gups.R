#!/usr/bin/env Rscript
library(sqldf)
library(ggplot2)
library(scales)
#library(reshape)

# if you want to load external fonts (e.g. Humor Sans)
# library(extrafont)
# loadfonts()

source("../common.R")

## db <- function(query) {
##   d <- sqldf(query, dbname="sosp.db")
  
##   #d$nnode <- factor(d$nnode) # make field categorical
##   d$ppn   <- factor(d$ppn)
##   d$periodic_poll_ticks <- factor(d$periodic_poll_ticks)
##   d$num_starting_workers <- factor(d$num_starting_workers)
##   d$loop_threshold <- factor(d$loop_threshold)
##   return(d)
## }
setwd('C:/cygwin/home/bdmyers/grappa/doc/sosp13/results/gups')
dd <- read.csv(file='gups.csv',header=TRUE,sep=",")
#ddd = melt(x, id.vars=1)


p <- ggplot(dd, aes(x=Nodes, y=GUPs, color=Aggregation))+
  #aes(x=nnode, y=mops_total, ymax=2048, group=problem, color=version, shape=ppn, label=mops_total))+
  ggtitle("GUPS")+
  sosp_theme+
  opts(aspect.ratio=.65,legend.position=c(0.8,0.6))+
  geom_line()+
  geom_point()+ # show points
  # stat_summary(fun.y="max", geom="line")+ # show line with max of each line
  #scale_y_continuous(trans=log2_trans())+
  scale_x_continuous(breaks=seq(0,128,16))
  
p # plot it
ggsave("gups.pdf", plot=p      ) # plot it

