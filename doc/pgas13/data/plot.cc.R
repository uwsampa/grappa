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

d <- db(
  "select * from cc
   ",
  c("nnode", "ppn",
    "num_starting_workers",
    "aggregator_autoflush_ticks",
    "periodic_poll_ticks",
    "flat_combining",
    "cc_concurrent_roots"
  )
)

d$ops_per_msg <- with(d, hashset_insert_ops/hashset_insert_msgs)
d$throughput <- with(d, (2**scale*16)/components_time)

d.m <- melt(d, measure=c("ops_per_msg", "throughput"))

g <- ggplot(d.m, aes(
    x=num_starting_workers,
    y=value,
    color=cc_concurrent_roots,
    shape=flat_combining,
    label=cc_set_size
  ))+
  geom_point()+
  geom_text(size=2,hjust=-0.2)+
  facet_grid(~variable~nnode~ppn~scale, scales="free", labeller=label_pretty)+
  ylab("")+
  expand_limits(y=0)+
  my_theme
g

d.t <- melt(d, measure=c("cc_propagate_time","cc_reduced_graph_time","cc_set_insert_time"))
d.t$cc_time_names <- d.t$variable
d.t$cc_time_values <- d.t$value

g.t <- ggplot(d.t,
  aes(
    x=cc_concurrent_roots,
    y=cc_time_values,
    color=cc_time_names, fill=cc_time_names
  ))+
  facet_grid(~flat_combining~num_starting_workers, labeller=label_value)+
  geom_bar(stat="identity")+
  my_theme
g.t