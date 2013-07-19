#!/usr/bin/env Rscript
library(sqldf)
library(ggplot2)
library(scales)
library(grid)
#library(reshape)

#source("../common.R")

#d.intsort <- sqldf("select * from histograms where core == 1 and stat == 'app_bytes_sent_histogram' and value > 0", dbname="hist.intsort.db")
# d.intsort <- sqldf("select * from histograms where core == 1 and stat == 'rdma_bytes_sent_histogram' and value > 0", dbname="hist.intsort.db")
# d.intsort <- sqldf("select * from histograms where core == 1 and stat == 'delegate_op_latency_histogram' and value > 0", dbname="hist.intsort.db")
d.intsort <- sqldf("select * from histograms", dbname="hist.intsort.db")

#d.intsort <- sample("hist.intsort.db",1000)
#d.intsort$benchmark <- "intsort"

#d.bfs <- sqldf("select * from histograms where core % 8 == 0", dbname="hist.bfs.db")
#d.bfs <- sample("hist.bfs.db",1000)
# d.bfs$benchmark <- "bfs"


hist <- function(data,title,maxx,bin) {
  p <- ggplot(data, aes(x=value))+
    ggtitle(title)+
    sosp_theme+
    geom_histogram(binwidth=bin)+
    coord_cartesian(xlim=c(0,maxx))+
    facet_grid(~stat,labeller=label_both)
  return(p)
}

# plot_rdma_bytes <- function(data,title) {
#   p <- ggplot(subset(data,stat == 'app_bytes_sent_histogram'), aes(x=value))+
#     ggtitle(title)+
#     sosp_theme+
#     geom_histogram(binwidth=2000)+
#     coord_cartesian(xlim=c(0,64000))+
#     facet_grid(~stat,labeller=label_both)
#   return(p)
# }
# 
# plot_app_bytes <- function(data,title) {
#   p <- ggplot(subset(data,stat == 'app_bytes_sent_histogram'), aes(x=value))+
#     ggtitle(title)+
#     sosp_theme+
#     geom_histogram(binwidth=20)+
#     coord_cartesian(xlim=c(0,5000))+
#     facet_grid(~stat,labeller=label_both)
#   return(p)
# }
# 
# plot_latency <- function(data,title) {
#   p <- ggplot(subset(data,stat == 'app_bytes_sent_histogram'), aes(x=value))+
#     ggtitle(title)+
#     sosp_theme+
#     geom_histogram(binwidth=20)+
#     coord_cartesian(xlim=c(0,5000))+
#     facet_grid(~stat,labeller=label_both)
#   return(p)
# }


# p.bfs <- hist(d.bfs,"BFS App bytes",500,10)
d.bfs <- sqldf("select * from histograms where core == 1 and stat == 'rdma_bytes_sent_histogram' and value > 0", dbname="hist.bfs.db")
# ggsave("plots/rdma_bytes_sent_histogram_bfs.pdf", plot=hist(d.bfs, "RDMA Bytes Sent - BFS", 64000, 2000))
# 
# d.intsort <- sqldf("select * from histograms where core == 1 and stat == 'rdma_bytes_sent_histogram' and value > 0", dbname="hist.intsort.db")
# ggsave("plots/rdma_bytes_sent_histogram_intsort.pdf", plot=hist(d.intsort, "RDMA Bytes Sent - IntSort", 64000, 2000))
# 
# d.uts <- sqldf("select * from histograms where core == 1 and stat == 'rdma_bytes_sent_histogram' and value > 0", dbname="hist.uts.db")
# ggsave("plots/rdma_bytes_sent_histogram_uts.pdf", plot=hist(d.uts, "RDMA Bytes Sent - UTS", 64000, 2000))

# d.pagerank <- sqldf("select * from histograms where stat == 'rdma_bytes_sent_histogram' and value > 0", dbname="hist.pagerank.db")
# ggsave("plots/rdma_bytes_sent_histogram_pagerank.pdf", plot=hist(d.pagerank, "RDMA Bytes Sent - PageRank", 64000, 2000))
d.combined <- sqldf("select * from histograms where stat == 'rdma_bytes_sent_histogram' and value > 0", dbname="comb.db")
d.combined$benchmark <- factor(d.combined$benchmark)

p.cmb <- ggplot(d.combined, aes(x=value))+
  #ggtitle("Aggregated message sizes")+
  sosp_theme+xlab("Size of aggregated message (in bytes)")+ylab("Normalized count")+
  geom_histogram(aes(y=..count../sum(..count..)),binwidth=2000)+
  coord_cartesian(xlim=c(0,64000))+
  theme(panel.margin = unit(1, "lines"))+
  facet_grid(~benchmark)

ggsave("plots/rdma_bytes_sent_histogram_cmb.pdf", plot=p.cmb)

d.c <- sqldf("select value/2.1e9*1e3 as millis, * from histograms where stat == 'delegate_op_latency_histogram' and value > 0", dbname="comb.db")
d.combined$benchmark <- factor(d.combined$benchmark)

p.c <- ggplot(d.c, aes(x=millis))+
  #ggtitle("Delegate operation latencies")+
  sosp_theme+xlab("Latency (ms)")+ylab("Normalized count")+
  # geom_histogram(binwidth=500000)+
  geom_histogram(aes(y=..count../sum(..count..)),binwidth=.4)+
  coord_cartesian(xlim=c(0,20))+
  theme(panel.margin = unit(1, "lines"))+
  facet_grid(~benchmark,scales="free_y")

ggsave("plots/latency_cmb.pdf", plot=p.c)




# d.bfs.app <- sqldf("select * from histograms where core == 1 and stat == 'app_bytes_sent_histogram' and value > 0", dbname="hist.bfs.db")
# ggsave("plots/app_bytes_sent_histogram_bfs.pdf", plot=hist(d.bfs.app,"App Bytes Sent - BFS", 5000, 10))

# d.bfs.lat <- sqldf("select * from histograms where core == 500 and stat == 'delegate_op_latency_histogram' and value > 0", dbname="hist.bfs.db")
# ggsave("plots/delegate_op_latency_histogram_bfs.500.pdf", plot=hist(d.bfs.lat,"Delegate Op Latency - BFS", 50000000, 500000))
# 
# d.intsort.lat <- sqldf("select * from histograms where core == 500 and stat == 'delegate_op_latency_histogram' and value > 0", dbname="hist.intsort.db")
# ggsave("plots/delegate_op_latency_histogram_intsort.500.pdf", plot=hist(d.intsort.lat,"Delegate Op Latency - IntSort", 50000000, 500000))
# 
# d.pagerank.lat <- sqldf("select * from histograms where core == 500 and stat == 'delegate_op_latency_histogram' and value > 0", dbname="hist.pagerank.db")
# ggsave("plots/delegate_op_latency_histogram_pagerank.500.pdf", plot=hist(d.pagerank.lat,"Delegate Op Latency - BFS", 50000000, 500000))
# 
# d.uts.lat <- sqldf("select * from histograms where core == 500 and stat == 'delegate_op_latency_histogram' and value > 0", dbname="hist.uts.db")
# ggsave("plots/delegate_op_latency_histogram_uts.500.pdf", plot=hist(d.uts.lat,"Delegate Op Latency - BFS", 50000000, 500000))



#       642245067

#
# p.intsort <- hist(d.intsort,"IntSort App bytes",5000,100)
# p.intsort
# p.intsort <- sosp_histogram(d.intsort)
#ggsave("plots/rdma_bytes_sent_histogram_intsort.pdf", plot=sosp_histogram(d.intsort))
# ggsave("plots/rdma_bytes_sent_histogram_pagerank.pdf", plot=sosp_histogram(d.pagerank))
# ggsave("plots/rdma_bytes_sent_histogram_uts.pdf", plot=sosp_histogram(d.))


#p.combined <- sosp_histogram(d.combined)
#ggsave("plots/rdma_bytes_sent_histogram_combined.pdf", plot=p.combined)

# p.labeled = p+geom_text(size=4, hjust=-0.1)
