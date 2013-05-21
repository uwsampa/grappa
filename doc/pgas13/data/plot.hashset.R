#!/usr/bin/env Rscript
library(sqldf)
library(ggplot2)
library(reshape)
library(extrafont)
loadfonts()
source("common.R")

db <- function(query, factors, db) {
  d <- sqldf(query, dbname=db)
  d[factors] <- lapply(d[factors], factor)
  return(d)
}

# d <- sqldf("select * from test where ppn == 16 and nnode == 8 and scale == 28", dbname="test.db")
# f <- c("nnode", "num_starting_workers", "aggregator_autoflush_ticks", "periodic_poll_ticks")
# d[f] <- lapply(d[f], factor)
d <- db("select * from hashset where log_nelems == 28",
      c("nnode",
        "num_starting_workers",
        "aggregator_autoflush_ticks",
        "periodic_poll_ticks",
        "flat_combining",
        "log_max_key",
        "log_nelems"
      ), "pgas.sampa.sqlite")

d$ops_per_msg <- with(d, hashset_insert_ops/hashset_insert_msgs)
d$throughput <- with(d, hashset_insert_ops/hashset_insert_time_mean)

d <- melt(d, measure=c("ops_per_msg", "throughput"))

g <- ggplot(d, aes(
    x=num_starting_workers,
    y=value,
    color=log_max_key,
    shape=flat_combining
    # label=nnode~ppn,
  ))+
  geom_point()+
  facet_grid(~variable~nnode~ppn~log_nelems, scales="free", labeller=label_pretty)+
  ylab("")+
  expand_limits(y=0)+
  my_theme

g

