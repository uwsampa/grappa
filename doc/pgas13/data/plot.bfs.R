#!/usr/bin/env Rscript
library(sqldf)
library(ggplot2)
library(reshape)
library(extrafont)
loadfonts()
source("common.R")

db <- function(query) {
  d <- sqldf(query, dbname="pgas.sqlite")
  factors <- c("nnode", "num_starting_workers")
  d[factors] <- lapply(d[factors], factor)
  return(d)
}

d <- db("select * from bfs where global_vector_push_ops > 0")
d$global_vector_push_ops_per_msg <- with(d, global_vector_push_ops/global_vector_push_msgs)

d <- melt(d, measure=c("global_vector_push_ops_per_msg", "max_teps"))

g <- ggplot(d, aes(
    x=num_starting_workers,
    y=value,
    color=flat_combining
    # label=nnode~ppn,
  ))+
  geom_point()+
  facet_grid(~variable~ppn, scales="free", labeller=label_bquote(.(prettify(x))))+
  my_theme

g
