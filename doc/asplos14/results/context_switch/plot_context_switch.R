#!/usr/bin/env Rscript
setwd("C:/cygwin/home/bdmyers/grappa/doc/rplotting/examples")

library(sqldf)
library(ggplot2)
library(scales)
#library(reshape)

source("../common.R")
dbpath <- "C:/cygwin/home/bdmyers/results_softxmt/"

db <- function(query) {
  d <- sqldf(query, dbname=paste(dbpath, "threads/context_switch-final.db", sep=''))
       
  # always treat as categorical
  d$worker_prefetch_amount <- as.factor(d$worker_prefetch_amount)
  d$stack_prefetch_amount <- as.factor(d$stack_prefetch_amount)
  
  # renaming
  levels(d$stack_prefetch_amount)[levels(d$stack_prefetch_amount)==0] <- "No prefetching"
  levels(d$stack_prefetch_amount)[levels(d$stack_prefetch_amount)==3] <- "Prefetching"
  return (d)
}

# to make groups for lines
grouping <- function(x, xvals, y, yvals) {
  category <- 0
  for (i in xvals) {
    for (j in yvals) {
      if (x==i & y==j) {
        return (category)
      } else {
        category <- category+1
      }
    }
  }
  return (0+"x,y not supported")
}

f <- db("select * from context_switch 
                    where problem_flavor='new-worker-multi-queue-prefetch2x-fixdq-pincpualt'
                    AND (ppn=1 OR ppn=6)")
# create a grouping so that we can have numLines=cardinality(cross product(ppn,stack_prefetch_amount))
ppns.to.use <- c(1,6)
for (i in 1:dim(f)[1]) {
  f$line_group[i] <- grouping(f$ppn[i], ppns.to.use, f$stack_prefetch_amount[i], levels(f$stack_prefetch_amount))
}
with.without.1.6 <- ggplot(f,
                 aes(x=num_test_workers*ppn, 
                     y=context_switch_test_runtime_avg*1e9/actual_total_iterations,
                     group = line_group,
                     shape=factor(ppn),
                     linetype=stack_prefetch_amount))+
                       ggtitle("Context switch")+
                       sosp_theme+
                       scale_shape_discrete(name="Number\nof cores")+    # core legend title
                       scale_linetype_discrete(name="")+  # prefetching legend has no title
                       #geom_point(size=2.5)+ # show all unaggregated points
                       #geom_text(data=f[f$ppn==6 & f$node_num_workers>100000 & f.ppn1.6$node_num_workers<160000,], label="6 cores", vjust=1)+
                       stat_summary(fun.y=mean, geom="line", size = 1 )+
                       stat_summary(fun.y=mean, geom="point", size = 3 )+
                      scale_x_continuous(name="Number of workers", trans=log2_trans())+  # x-axis
                      scale_y_continuous(name="Avg context switch latency (ns)") # y-axis

with.without.1.6 # plot p
ggsave("plots/context_switch_time.pdf", plot=with.without.1.6)

f <- db("select * from context_switch 
                    where problem_flavor='new-worker-multi-queue-prefetch2x-fixdq-pincpualt'
                    AND (ppn=1 OR ppn=6 OR ppn=4 OR ppn=2) AND stack_prefetch_amount==3")
bandwidth <- ggplot(f,
                    aes(x=num_test_workers*ppn,
                        y=actual_total_iterations*ppn/context_switch_test_runtime_avg/1000000,
                        group=factor(ppn),
                        color=factor(ppn),
                        linetype=factor(ppn)))+
                    ggtitle("Context switch")+
                    sosp_theme+
                      scale_linetype_discrete(name="Number\nof cores")+  # prefetching legend has no title
                      scale_colour_discrete(guide='none')+
                      stat_summary(fun.y=mean, geom="line", size = 1 )+
                      scale_x_continuous(name="Number of workers", trans=log2_trans())+  # x-axis
                      scale_y_continuous(name="Context switch / second (millions)") # y-axis

bandwidth


