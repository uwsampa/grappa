#!/usr/bin/env Rscript
library(sqldf)
library(ggplot2)
library(scales)
library(reshape)

# if you want to load external fonts (e.g. Humor Sans)
# library(extrafont)
# loadfonts()

source("../common.R")

db <- function(query) {
  d <- sqldf(query, dbname="sosp.db")
  
  #d$nnode <- factor(d$nnode) # make field categorical
  d$ppn   <- factor(d$ppn)
  d$periodic_poll_ticks <- factor(d$periodic_poll_ticks)
  d$num_starting_workers <- factor(d$num_starting_workers)
  d$loop_threshold <- factor(d$loop_threshold)
  return(d)
}

stat_sum_single <- function(fun, geom="text", ...) {
  stat_summary(fun.y=fun, geom=geom, size = 3, ...)
}

p <- ggplot(db("select * from intsort
               where nnode <= 128
               and problem == 'E'
               "),
  aes(x=nnode, y=mops_total, ymax=2048, group=version, color=version, label=mops_total))+
  ggtitle("IntSort")+ylab("MOps/second")+xlab("Nodes")+
  sosp_theme+
  geom_point()+ # show points
  # stat_summary(fun.y="max", geom="line")+ # show line with max of each line
  # scale_y_continuous(trans=log2_trans())+
  scale_x_continuous(breaks=seq(0,128,16))+
  facet_wrap(~problem) # make new plots for each scale
  
p # plot it

#p.labeled = p+stat_summary(fun.y=max, geom="text", mapping=aes(y=mops_total,group=version))
p.labeled = p+stat_summary(fun.y=max, geom="text", size=4)

ggsave("plots/intsort_mops.pdf", plot=p.labeled, scale=1.3) # or save it

d <- db("select *,nnode*ppn as nproc from intsort")
p.nproc <- ggplot(d, aes(x=nproc, y=mops_total, group=problem, color=problem))+
  ggtitle("IntSort")+xlab("Cores")+ylab("MOPS")+
  scale_x_continuous(breaks=seq(0, 2048, 512))+ # axis ticks seq(from,to,by)
  sosp_theme+
  geom_point()+ # show points
  # stat_summary(fun.y="max", geom="label")+ # show line with max of each line
  facet_wrap(~problem) # make new plots for each scale
p.nproc
#ggsave("plots/intsort_nproc.pdf", plot=p.nproc) # plot it


p.grappa <- ggplot(db(
  " select *, nnode*ppn as nproc from intsort
    where loop_threshold > 0
    and nnode == 8 and ppn == 2
  "), aes(x=periodic_poll_ticks, y=mops_total, group=num_starting_workers, color=num_starting_workers, shape=ppn))+
  ggtitle("IntSort")+
  #scale_x_continuous(breaks=seq(0, 2048, 512))+ # axis ticks seq(from,to,by)
  sosp_theme+
  geom_point()+ # show points
  geom_line()+
  # stat_summary(fun.y="max", geom="label")+ # show line with max of each line
  facet_wrap(~problem~nnode~ppn) # make new plots for each scale
p.grappa
#ggsave("plots/intsort_nproc.pdf", plot=p.nproc) # plot it


p.times <- ggplot(db(
  " select *, nnode*ppn as nproc from intsort
    where loop_threshold > 0
    and nnode == 8 and ppn == 2
  "), aes(x=periodic_poll_ticks, y=total_time, group=num_starting_workers, color=num_starting_workers, shape=ppn))+
  ggtitle("IntSort")+
  #scale_x_continuous(breaks=seq(0, 2048, 512))+ # axis ticks seq(from,to,by)
  sosp_theme+
  geom_point()+ # show points
  geom_line()+
  geom_line(mapping=aes(y=allreduce_time,color=blue))+
  # stat_summary(fun.y="max", geom="label")+ # show line with max of each line
  facet_wrap(~problem~nnode~ppn) # make new plots for each scale

p.thresh <- ggplot(db(
  " select *, nnode*ppn as nproc from intsort
    where loop_threshold > 0
    and nnode == 8 and ppn == 2
  "), aes(x=loop_threshold, y=mops_total, group=num_starting_workers, color=num_starting_workers, shape=periodic_poll_ticks))+
  ggtitle("IntSort")+
  #scale_x_continuous(breaks=seq(0, 2048, 512))+ # axis ticks seq(from,to,by)
  sosp_theme+
  geom_point()+ # show points
  geom_line()+
  # stat_summary(fun.y="max", geom="label")+ # show line with max of each line
  facet_wrap(~problem~nnode~ppn) # make new plots for each scale
