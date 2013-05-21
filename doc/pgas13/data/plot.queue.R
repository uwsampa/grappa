#!/usr/bin/env Rscript
library(sqldf)
library(ggplot2)
library(reshape)
library(extrafont)
loadfonts()
source("common.R")

db <- function(query, factors, db="pgas.sqlite") {
  d <- sqldf(query, dbname=db)
  d[factors] <- lapply(d[factors], factor)
  return(d)
}

d <- db("select * from queue where ppn == 16 and nnode == 8 and scale == 28 and aggregator_autoflush_ticks == 100000 and loop_threshold == 64", c("nnode", "num_starting_workers", "aggregator_autoflush_ticks", "periodic_poll_ticks", "flat_combining"))

d$ops_per_msg <- with(d, global_vector_push_ops/global_vector_push_msgs)
d$throughput <- with(d, global_vector_push_ops/push_time_mean)

d <- melt(d, measure=c("ops_per_msg", "throughput"))

g <- ggplot(d, aes(
    x=num_starting_workers,
    y=value,
    color=flat_combining,
    shape=aggregator_autoflush_ticks
    # label=nnode~ppn,
  ))+
  geom_point()+
  facet_grid(~variable~nnode~ppn~scale, scales="free", labeller=label_pretty)+
  ylab("")+
  expand_limits(y=0)+
  my_theme

g

