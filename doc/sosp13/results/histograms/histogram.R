#!/usr/bin/env Rscript
library(sqldf)
library(ggplot2)
library(scales)
#library(reshape)

#source("../common.R")

set.seed(1234)
df <- data.frame(cond = factor( rep(c("A","B"), each=200) ), 
                 rating = c(rnorm(200),rnorm(200, mean=.8)))

p <- ggplot(df, aes(x=rating))+
#  aes(x=nnode, y=mops_total, ymax=2048, group=problem, color=version, shape=ppn, label=mops_total))+
  ggtitle("Histogram")+
#  sosp_theme+
  geom_histogram(binwidth=0.5)
#  geom_point()+ # show points
  # stat_summary(fun.y="max", geom="line")+ # show line with max of each line
  # scale_y_continuous(trans=log2_trans())+
# scale_x_continuous(breaks=seq(0,128,16))
# facet_wrap(~problem) # make new plots for each scale
  
p # plot it

p.labeled = p+geom_text(size=4, hjust=-0.1)
