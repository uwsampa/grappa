#!/usr/bin/env Rscript
setwd("~/dev/hub/grappa/doc/pgas13/data")
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
        "insert_async",
        "fraction_lookups"
      ), "pgas.sqlite")

dmap <- db("select * from hashmap where log_nelems == 28",
      c("nnode",
        "num_starting_workers",
        "aggregator_autoflush_ticks",
        "periodic_poll_ticks",
        "flat_combining",
        "log_max_key",
        "log_nelems",
        "insert_async",
        "fraction_lookups"
      ), "pgas.sqlite")


d$ops_per_msg <- with(d, (hashset_insert_ops+hashset_lookup_ops)/(hashset_insert_msgs+hashset_lookup_msgs))
d$throughput <- with(d, (hashset_insert_ops+hashset_lookup_ops)/trial_time_mean)

d$fc_version <- sapply(paste('v',d$flat_combining,d$insert_async,sep=''),switch,
  v0NA='none', v1NA='fc', v11='async', v10='fc', v00='none', v01='?'
)

d.melt <- melt(d, measure=c("ops_per_msg", "throughput"))

g <- ggplot(subset(d.melt,
    version == 'fc_looks'
  ), aes(
    x=num_starting_workers,
    y=value,
    color=x(log_max_key,fraction_lookups),
    shape=fc_version,
    group=x(log_max_key,fraction_lookups,fc_version),
    label=trial_time_mean,
    linetype=fc_version,
  ))+
  geom_point()+
  # geom_text()+
  geom_line()+
  # geom_smooth(aes(linetype=fc_version), fill=NA)+
  facet_grid(variable~log_nelems~nnode, scales="free", labeller=label_pretty)+
  ylab("")+
  expand_limits(y=0)+
  my_theme+theme(strip.text=element_text(size=rel(0.4)),
                 axis.text.x=element_text(size=rel(0.65)))
ggsave(plot=g, filename="plots/hashset_thru.pdf", width=16, height=10)


gg <- ggplot(subset(d, log_nelems==28 & ppn==16 & num_starting_workers==8192
                    & log_max_key == 14 & version == 'fc_looks'
  ),aes(
    x=nnode,
    y=throughput,
    color=fc_version,
    group=x(fc_version,version,fraction_lookups,trial),
    shape=fraction_lookups,
    linetype=fraction_lookups,
  ))+
  geom_point()+
  geom_line()+
  # facet_grid(log_nelems~., scales="free", labeller=label_pretty)+
  xlab("Nodes")+
  scale_color_discrete(name="Combining mode:")+
  scale_linetype_discrete(name="Fraction lookups:")+
  scale_shape_discrete(name="Fraction lookups:")+
  ylab("Throughput")+expand_limits(y=0)+my_theme+
  theme(strip.text=element_text(size=rel(0.4)),
        axis.text.x=element_text(size=rel(0.65)))

ggsave(plot=gg, filename="plots/hashset_perf.pdf", width=8, height=6)

######################
# GlobalHashMap

dmap$ops_per_msg <- with(dmap, (hashmap_insert_ops+hashmap_lookup_ops)/(hashmap_insert_msgs+hashmap_lookup_msgs))
dmap$throughput <- with(dmap, (hashmap_insert_ops+hashmap_lookup_ops)/trial_time_mean)
dmap$fc_version <- sapply(paste('v',dmap$flat_combining,dmap$insert_async,sep=''),switch,
  v0NA='none', v1NA='fc', v11='async', v10='fc', v00='none', v01='?'
)

gg <- ggplot(subset(dmap, log_nelems==28 & ppn==16 & num_starting_workers==8192
                    & log_max_key == 14 & version == 'fc_looks'
  ),aes(
    x=nnode,
    y=throughput,
    color=fc_version,
    group=x(fc_version,version,fraction_lookups),
    shape=fraction_lookups,
    linetype=fraction_lookups,
  ))+
  geom_point()+
  geom_line()+
  # facet_grid(log_nelems~., scales="free", labeller=label_pretty)+
  xlab("Nodes")+
  scale_color_discrete(name="Combining mode:")+
  scale_linetype_discrete(name="Fraction lookups:")+
  scale_shape_discrete(name="Fraction lookups:")+
  ylab("Throughput")+expand_limits(y=0)+my_theme+
  theme(strip.text=element_text(size=rel(0.7)),
        axis.text.x=element_text(size=rel(0.85)))

ggsave(plot=gg, filename="plots/hashmap_perf.pdf", width=8, height=6)

