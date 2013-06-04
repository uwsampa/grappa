#!/usr/bin/env Rscript
setwd("~/dev/hub/grappa/doc/pgas13/data")
source("common.R")

db <- function(query, factors, db="pgas.sqlite") {
  d <- sqldf(query, dbname=db)
  d[factors] <- lapply(d[factors], factor)
  return(d)
}

d <- db("select * from queue where ppn == 16",
      c("nnode", "num_starting_workers", "aggregator_autoflush_ticks", "periodic_poll_ticks", "flat_combining","fraction_push","ppn"))

d$ops_per_msg <- with(d, (global_vector_push_ops+global_vector_deq_ops)/(global_vector_push_msgs+global_vector_deq_msgs))
d$throughput <- with(d, nelems/trial_time_mean)

d$fc_version <- sapply(paste('v',d$flat_combining,d$flat_combining_local_only,sep=''),switch,
  v0NA='none', v1NA='full', v10='full', v11='local', v00='none'
)

d.melt <- melt(d, measure=c("ops_per_msg", "throughput"))

g <- ggplot(subset(d.melt, version == 'fixed_random'), aes(
    x=num_starting_workers,
    y=value,
    color=x(flat_combining,flat_combining_local_only,fraction_push),
    shape=x(ppn,version),
    group=x(flat_combining,fraction_push,ppn,version)
    # label=nnode~ppn,
  ))+
  geom_point()+
  geom_line()+
  facet_grid(variable~log_nelems+nnode, scales="free", labeller=label_pretty)+
  ylab("")+expand_limits(y=0)+my_theme+
  theme(strip.text=element_text(size=rel(0.4)),
        axis.text.x=element_text(size=rel(0.65)))

# ggsave(plot=g, filename="plots/queue_thru.pdf", width=16, height=10)

gg <- ggplot(subset(d, log_nelems==28 & ppn==16 & num_starting_workers==2048
                      & version == 'fixed_random'
  ),aes(
    x=as.numeric(as.character(nnode)),
    y=throughput,
    color=fc_version,
    group=x(fc_version,version,fraction_push,trial),
    shape=fc_version,
    linetype=fraction_push,
  ))+
  geom_point(size=3)+
  geom_line(size=1)+
  # facet_grid(log_nelems~., scales="free", labeller=label_pretty)+
  xlab("Nodes")+scale_x_continuous(breaks=c(0,8,16,32,48,64))+
  scale_color_discrete(name="Flat Combining")+
  scale_shape_discrete(name="Flat Combining")+
  scale_linetype_discrete(name="Fraction pushes")+
  ylab("Throughput (ops/sec)")+expand_limits(y=0)+my_theme+
  theme(strip.text=element_text(size=rel(0.4)),
        axis.text.x=element_text(size=rel(0.75)))

ggsave(plot=gg, filename="plots/queue_perf.pdf", width=7, height=5)

