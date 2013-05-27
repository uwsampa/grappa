#!/usr/bin/env Rscript
library(sqldf)
library(ggplot2)
library(reshape)
library(extrafont)
loadfonts()
source("common.R")

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
        "log_nelems",
        "insert_async"
      ), "pgas.sqlite")

d$ops_per_msg <- with(d, hashset_insert_ops/hashset_insert_msgs)
d$throughput <- with(d, hashset_insert_ops/hashset_insert_time_mean)

d$fc_version <- sapply(paste('v',d$flat_combining,d$insert_async,sep=''),switch,
  v0NA='none', v1NA='fc', v11='async', v10='fc', v00='none', v01='?'
)

d <- melt(d, measure=c("ops_per_msg", "throughput"))

g <- ggplot(d, aes(
    x=num_starting_workers,
    y=value,
    color=x(log_max_key,fraction_lookups),
    shape=fc_version,
    group=x(log_max_key,fraction_lookups,fc_version)
    # label=nnode~ppn,
  ))+
  geom_point()+
  geom_smooth(aes(linetype=fc_version), fill=NA)+
  facet_grid(variable~log_nelems~nnode, scales="free", labeller=label_pretty)+
  ylab("")+
  expand_limits(y=0)+
  my_theme

ggsave(plot=g, filename="plots/hashset_thru.pdf", scale=1.3)

