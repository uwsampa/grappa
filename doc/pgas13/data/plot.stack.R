#!/usr/bin/env Rscript
setwd("~/dev/hub/grappa/doc/pgas13/data")
source("common.R")

db <- function(query, factors, db="pgas.sqlite") {
  d <- sqldf(query, dbname=db)
  d[factors] <- lapply(d[factors], factor)
  return(d)
}

d <- db("select * from stack where ppn == 16",
      c("nnode", "num_starting_workers", "aggregator_autoflush_ticks", "periodic_poll_ticks", "flat_combining", "fraction_push"))
dq <- db("select * from queue where ppn == 16",
    c("nnode", "num_starting_workers", "aggregator_autoflush_ticks", "periodic_poll_ticks", "flat_combining", "fraction_push"))

d$ops_per_msg <- with(d, (global_vector_push_ops+global_vector_pop_ops)/(global_vector_push_msgs+global_vector_pop_msgs))
d$throughput <- with(d, nelems/trial_time_mean)

dq$ops_per_msg <- with(dq, (global_vector_push_ops+global_vector_pop_ops)/(global_vector_push_msgs+global_vector_pop_msgs))
dq$throughput <- with(dq, nelems/trial_time_mean)

d$fc <- with(d, x(flat_combining,flat_combining_local_only))

d$fc_version <- sapply(paste('v',d$flat_combining,d$flat_combining_local_only,sep=''),switch,
  v0NA='none', v1NA='full', v10='full', v11='distributed', v00='centralized', v01='none'
)
dq <- within(dq,
fc_version <- sapply(paste('v',flat_combining,flat_combining_local_only,sep=''),switch,
  v0NA='none', v1NA='full', v10='full', v11='distributed', v00='centralized', v01='none'
))


d.melt <- melt(d, measure=c("global_vector_push_latency", "global_vector_pop_latency", "ops_per_msg", "throughput"))

g <- ggplot(subset(d.melt,
    # & nnode == 16 
    log_nelems == 28 & ppn == 16 & num_starting_workers == 2048
    & ( (( fraction_push != 0.5) & version == 'fixed_random')
      | version == 'matching_better'
      )
  ), aes(
    x=nnode,
    y=value,
    color=fc_version,
    shape=version,
    group=x(fc_version,fraction_push,version),
    linetype=fraction_push,
    # label=global_vector_buffer,
  ))+
  geom_point()+
  geom_line()+
  # geom_text()+
  facet_grid(variable~log_nelems, scales="free", labeller=label_pretty)+
  ylab("")+expand_limits(y=0)+my_theme+
  theme(strip.text=element_text(size=rel(0.4)),
        axis.text.x=element_text(size=rel(0.65)))

ggsave(plot=g, filename="plots/stack_thru.pdf", width=12, height=10)

d$nnode <- as.numeric(as.character(d$nnode))
g <- ggplot(subset(d,
      log_nelems==28 & ppn==16 & num_starting_workers==2048
    & ((version == 'fixed_random' & fraction_push != 0.5) 
    | (version == 'matching_better' & fraction_push == 0.5)
    | (version == 'with_master_only'))
  ), aes(
    x=nnode,
    y=throughput,
    color=fc_version,
    group=x(fc_version,version,fraction_push),
    shape=fc_version,
    linetype=fraction_push,
  ))+
  geom_point(size=3)+
  geom_line(size=1)+
  xlab("Nodes")+scale_x_continuous(breaks=c(0,8,16,32,48,64))+
  scale_y_log10()+
  scale_color_discrete(name="Flat Combining")+
  scale_shape_discrete(name="Flat Combining")+
  scale_linetype_discrete(name="Fraction pushes")+
  # facet_grid(.~fraction_push, scales="free", labeller=label_pretty)+
  ylab("Throughput (ops/sec)")+expand_limits(y=0)+my_theme+
  theme(strip.text=element_text(size=rel(0.4)),
        axis.text.x=element_text(size=rel(0.75)))

ggsave(plot=g, filename="plots/stack_perf.pdf", width=7, height=5)


#####################
# Combined
# d <- within(d,
#   mix <- sapply(fraction_push, switch, '1'='100% push', '0.5'='50% push, 50% pop', '?')
# )
# dq <- within(dq,
#   mix <- sapply(fraction_push, switch, '1'='100% pop', '0.5'='50% push, 50% pop', '?')
# )
dq$mix <- sapply(dq$fraction_push, function(f){ return(ifelse(f == '1', '100% push', '50% push,\n50% pop')) })
d$mix <- sapply(d$fraction_push, function(f){ return(ifelse(f == '1', '100% push', '50% push,\n50% pop')) })

d$struct <- sapply( d$nnode, function(i){ return('GlobalStack') })
dq$struct <- sapply(  dq$nnode, function(i){ return('GlobalQueue') })

d.c <- merge(dq, d, all=T)

# d.c$struct <- relevel(d.c$struct, 'GlobalStack')
# d.c$struct <- factor(d.c$struct, levels=rev(levels(d.c$struct)) )

gg <- ggplot(subset(d.c, 
  log_nelems==28 & ppn==16 & num_starting_workers==2048
  & ( ((struct == 'GlobalQueue' | fraction_push != 0.5) & version == 'fixed_random')
    | version == 'matching_better'
    )
  & fc_version != 'full'
  ),aes(
    x=nnode,
    y=throughput/1e6,
    color=fc_version,
    group=x(fc_version,version,fraction_push),
    linetype=mix,
    # shape=mix,
  ))+
  # geom_point(size=2.5)+
  # geom_line()+
  geom_smooth(size=1, fill=NA)+
  # facet_grid(log_nelems~., scales="free", labeller=label_pretty)+
  facet_grid(.~struct, scales="free_x")+
  scale_y_log10()+
  xlab("Nodes")+ylab("Throughput (millions of ops/sec)")+
  # scale_x_continuous(breaks=c(8,16,32,48,64))+
  # scale_color_discrete(name="Flat Combining")+
  # scale_color_brewer(name="Flat Combining", palette="Set1")+
  scale_color_manual(name="Flat Combining", values=c(color.green,color.red,color.blue))+
  scale_linetype_discrete(name="Operation Mix")+
  # scale_shape_discrete(name="Operation Mix")+
  expand_limits(y=0)+my_theme+
  guides(colour=guide_legend(order=1), linetype=guide_legend(order=2))+
  theme(strip.text=element_text(size=rel(1)),
        axis.text.x=element_text(size=rel(0.85)))

ggsave(plot=gg, filename="plots/vector_perf.pdf", width=8, height=5)

# d$total_ops <- with(d, global_vector_pop_ops+global_vector_push_ops)
# d$matches <- with(d, (global_vector_matched_pushes+global_vector_matched_pops))
# d$unmatched <- with(d, total_ops - matches)
# d$unmatched_fraction <- with(d, unmatched / total_ops)

d$nnode.n <- as.numeric(as.character(d$nnode))
d$ppn.n <- as.numeric(as.character(d$ppn))
d$throughput_per_core <- with(d, throughput / (nnode.n*ppn.n))

d.melt <- melt(d, measure=c("ops_per_msg", "throughput_per_core"))
levels(d.melt$variable) <- c('ops/message', 'ops/sec/core')

# & ( ((struct == 'GlobalQueue' | fraction_push != 0.5) & version == 'fixed_random')
gg <- ggplot(subset(d.melt, 
  log_nelems==28 & ppn==16 & num_starting_workers==2048
  & (( ( fraction_push != 0.5) & version == 'fixed_random')
    | version == 'matching_better')
  & grepl('none|distributed', fc_version)
  ),aes(
    x=nnode,
    y=value,
    color=fc_version,
    group=x(fc_version,version,fraction_push),
    linetype=mix,
    shape=fc_version,
  ))+
  # geom_point(size=2.5)+
  # geom_line()+
  geom_smooth(size=1, fill=NA)+
  # scale_fill_hue(h=c(90,100))+
  # scale_color_hue(h=c(90,100))+
  # scale_color_brewer(name="Flat Combining", palette="Set1")+
  # scale_color_manual(values=c("red", "blue", "green"))+
  # scale_color_gradient()+
  # facet_grid(log_nelems~., scales="free", labeller=label_pretty)+
  # facet_grid(variable~struct, scales="free")+
  facet_wrap(~variable, scales="free")+
  # scale_y_log10()+
  xlab("Nodes")+ylab("")+
  # scale_x_continuous(breaks=c(8,16,32,48,64))+
  scale_x_discrete(breaks=c(8,16,32,48,64))+
  # scale_color_discrete(name="Flat Combining", h=c(0,3))+
  scale_color_manual(name="Flat Combining", values=c(color.red, color.blue))+
  # scale_shape_discrete(name="Flat Combining")+
  scale_linetype_discrete(name="Operation Mix")+
  expand_limits(y=0)+my_theme+
  theme(strip.text=element_text(size=rel(1)),
        axis.text.x=element_text(size=rel(0.85)))

ggsave(plot=gg, filename="plots/stack_stats.pdf", width=8, height=5)

