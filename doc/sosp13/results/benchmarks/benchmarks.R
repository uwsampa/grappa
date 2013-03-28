#!/usr/bin/env Rscript
library(sqldf)
library(ggplot2)
library(scales)
library(reshape)

source("../common.R")

dd <- read.csv(file='input.csv',header=TRUE,sep=",")
x=dd

# argh
x[2] = dd[2]/dd[2]
x[3] = dd[3]/dd[2]
x[4] = dd[4]/dd[2]
x[5] = dd[5]/dd[2]
#x[6] = dd[6]/dd[2]

ddd = melt(x, id.vars=1)


p <- ggplot( ddd,
            aes(x=Platform,y=value,stat="identity")) +
  geom_bar(aes(fill=variable),position="dodge") +
  ggtitle("Benchmarks (normalized to Grappa)")+
  sosp_theme

ggsave("benchmarks.pdf", plot=p      ) # plot it
p
