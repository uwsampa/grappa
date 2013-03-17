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
  return(d)
}

p <- ggplot(db("select * from intsort"),
  aes(x=nnode, y=mops_total, group=problem, color=problem, shape=ppn))+
  ggtitle("IntSort")+xlab("Cores")+ylab("MOPS")+
  sosp_theme+
  geom_point()+ # show points
  # stat_summary(fun.y="max", geom="line")+ # show line with max of each line
  facet_wrap(~problem) # make new plots for each scale
  
p # plot it
ggsave("plots/intsort_mops.pdf", plot=p) # or save it

d <- db("select *,nnode*ppn as nproc from intsort")
p.nproc <- ggplot(d, aes(x=nproc, y=mops_total, group=problem, color=problem, shape=ppn))+
  ggtitle("IntSort")+xlab("Cores")+ylab("MOPS")+
  scale_x_continuous(breaks=seq(0, 2048, 512))+ # axis ticks seq(from,to,by)
  sosp_theme+
  geom_point()+ # show points
  geom_label(aes(y=))
  # stat_summary(fun.y="max", geom="label")+ # show line with max of each line
  facet_wrap(~problem) # make new plots for each scale
p.nproc
ggsave("plots/intsort_nproc.pdf", plot=p.nproc) # plot it
