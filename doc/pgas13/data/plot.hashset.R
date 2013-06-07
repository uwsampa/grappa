#!/usr/bin/env Rscript
setwd("~/dev/hub/grappa/doc/pgas13/data")
source("common.R")

# d <- sqldf("select * from test where ppn == 16 and nnode == 8 and scale == 28", dbname="test.db")
# f <- c("nnode", "num_starting_workers", "aggregator_autoflush_ticks", "periodic_poll_ticks")
# d[f] <- lapply(d[f], factor)
dset <- db("select * from hashset where log_nelems == 28",
      c("nnode",
        "num_starting_workers",
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
        "periodic_poll_ticks",
        "flat_combining",
        "log_max_key",
        "log_nelems",
        "insert_async",
        "fraction_lookups"
      ), "pgas.sqlite")

dq <- db("select * from queue where ppn == 16",
        c("nnode", "num_starting_workers", "aggregator_autoflush_ticks", "periodic_poll_ticks", "flat_combining","fraction_push","ppn"))
dst <- db("select * from stack where ppn == 16",
      c("nnode", "num_starting_workers", "aggregator_autoflush_ticks", "periodic_poll_ticks", "flat_combining","fraction_push","ppn"))

dset$ops_per_msg <- with(dset, (hashset_insert_ops+hashset_lookup_ops)/(hashset_insert_msgs+hashset_lookup_msgs))
dset$throughput <- with(dset, (hashset_insert_ops+hashset_lookup_ops)/trial_time_mean)

dset$fc_version <- sapply(paste('v',dset$flat_combining,dset$insert_async,sep=''),switch,
  v0NA='none', v1NA='local', v11='async', v10='local', v00='none', v01='?'
)

d.melt <- melt(dset, measure=c("ops_per_msg", "throughput"))

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
  # geom_point()+
  # geom_text()+
  geom_line()+
  # geom_smooth(aes(linetype=fc_version), fill=NA)+
  facet_grid(variable~log_nelems~nnode, scales="free", labeller=label_pretty)+
  ylab("")+
  expand_limits(y=0)+
  my_theme+theme(strip.text=element_text(size=rel(0.4)),
                 axis.text.x=element_text(size=rel(0.65)))
# ggsave(plot=g, filename="plots/hashset_thru.pdf", width=16, height=10)


gg <- ggplot(subset(dset, log_nelems==28 & ppn==16 & num_starting_workers==8192
                    & log_max_key == 14 & version == 'fc_looks'
  ),aes(
    x=nnode,
    y=throughput,
    color=fc_version,
    group=x(fc_version,version,fraction_lookups),
    shape=fraction_lookups,
    linetype=fraction_lookups,
  ))+
  # geom_point()+
  # geom_line()+
  geom_smooth(size=1)+
  # facet_grid(log_nelems~., scales="free", labeller=label_pretty)+
  xlab("Nodes")+
  scale_color_discrete(name="Flat Combining")+
  scale_linetype_discrete(name="Fraction lookups:")+
  scale_shape_discrete(name="Fraction lookups:")+
  ylab("Throughput (ops/sec)")+expand_limits(y=0)+my_theme+
  theme(strip.text=element_text(size=rel(0.7)),
        axis.text.x=element_text(size=rel(0.85)))

# ggsave(plot=gg, filename="plots/hashset_perf.pdf", width=7, height=5)

######################
# GlobalHashMap

dmap$ops_per_msg <- with(dmap, (hashmap_insert_ops+hashmap_lookup_ops)/(hashmap_insert_msgs+hashmap_lookup_msgs))
dmap$throughput <- with(dmap, (hashmap_insert_ops+hashmap_lookup_ops)/trial_time_mean)
dmap$fc_version <- sapply(paste('v',dmap$flat_combining,dmap$insert_async,sep=''),switch,
  v0NA='none', v1NA='local', v11='async', v10='local', v00='none', v01='?'
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
  # geom_point()+
  # geom_line()+
  geom_smooth(size=1)+
  # facet_grid(log_nelems~., scales="free", labeller=label_pretty)+
  xlab("Nodes")+
  # scale_x_continuous(breaks=c(8,16,32,48,64))+
  scale_color_discrete(name="Flat Combining")+
  scale_linetype_discrete(name="Fraction lookups:")+
  scale_shape_discrete(name="Fraction lookups:")+
  ylab("Throughput (ops/sec)")+expand_limits(y=0)+my_theme+
  theme(strip.text=element_text(size=rel(0.7)),
        axis.text.x=element_text(size=rel(0.85)))

# ggsave(plot=gg, filename="plots/hashmap_perf.pdf", width=7, height=5)


#####################
# Combined
dset <- within(dset,
  mix <- sapply(fraction_lookups, switch, '0'='100% insert', '0.5'='50% insert, 50% lookup')
  )
dmap <- within(dmap,
  mix <- sapply(fraction_lookups, switch, '0'='100% insert', '0.5'='50% insert, 50% lookup')
)

# dq <- within(dq,
#   mix <- sapply(sel('.',fraction_pushes), select, .1='all insert', .0.5='50% mix')
# )
# dst <- within(dst,
#   mix <- sapply(sel('.',fraction_pushes), select, .1='all push', .0.5='50% mix')
# )

# dmap$mix <- sapply(dmap$)
  dq$struct <- sapply(  dq$nnode, function(i){ return('GlobalQueue') })
 dst$struct <- sapply( dst$nnode, function(i){ return('GlobalQueue') })
dmap$struct <- sapply(dmap$nnode, function(i){ return('GlobalHashMap') })
dset$struct <- sapply(dset$nnode, function(i){ return('GlobalHashSet') })

d.c <- merge(dmap, dset, all=T)

gg <- ggplot(subset(d.c, log_nelems==28 & ppn==16 & num_starting_workers==8192
                    # & log_max_key == 14
                    & version == 'fc_looks_fixed'
                    & aggregator_autoflush_ticks == 500000
  ),aes(
    x=nnode,
    y=throughput/1e6,
    color=x(fc_version),
    group=x(fc_version,fraction_lookups),
    shape=mix,
    linetype=mix,
  ))+
  # geom_point()+
  # geom_line()+
  geom_smooth(size=1)+
  # facet_grid(log_nelems~., scales="free", labeller=label_pretty)+
  facet_grid(log_max_key~struct, scales="free_x")+
  xlab("Nodes")+ylab("Throughput (millions of ops/sec)")+
  # scale_x_continuous(breaks=c(8,16,32,48,64))+
  scale_color_discrete(name="Flat Combining")+
  scale_linetype_discrete(name="Fraction lookups")+
  scale_shape_discrete(name="Fraction lookups")+
  expand_limits(y=0)+my_theme+
  theme(strip.text=element_text(size=rel(1)),
        axis.text.x=element_text(size=rel(0.85)))

ggsave(plot=gg, filename="plots/hash_perf.pdf", width=7, height=4)

# d.c <- merge(dmap, dset, all=T)
# 
# gg <- ggplot(subset(d.c, log_nelems==28 & ppn==16 & num_starting_workers==8192
#                     & log_max_key == 14 & version == 'fc_looks'
#   ),aes(
#     x=nnode,
#     y=throughput,
#     color=fc_version,
#     group=x(fc_version,version,fraction_lookups),
#     shape=fraction_lookups,
#     linetype=fraction_lookups,
#   ))+
#   # geom_point()+
#   # geom_line()+
#   geom_smooth(size=1)+
#   # facet_grid(log_nelems~., scales="free", labeller=label_pretty)+
#   facet_grid(.~struct, scales="free_x")+
#   xlab("Nodes")+
#   # scale_x_continuous(breaks=c(8,16,32,48,64))+
#   scale_color_discrete(name="Flat Combining")+
#   scale_linetype_discrete(name="Fraction lookups")+
#   scale_shape_discrete(name="Fraction lookups")+
#   ylab("Throughput (ops/sec)")+expand_limits(y=0)+my_theme+
#   theme(strip.text=element_text(size=rel(0.7)),
#         axis.text.x=element_text(size=rel(0.85)))
# 
# ggsave(plot=gg, filename="plots/all.pdf", width=10, height=5)
# 
