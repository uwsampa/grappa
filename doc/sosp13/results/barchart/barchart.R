#!/usr/bin/env Rscript
library(sqldf)
library(ggplot2)
library(scales)
library(reshape)

source("../common.R")

dd <- read.csv(file='input.csv',header=TRUE,sep=",")
ddd = melt(dd[,c('PLATFORM','GRAPPA','MPI','XMT')], id.vars=1)
ggplot(ddd,aes(x=PLATFORM,y=value,stat="identity"))+geom_bar(aes(fill=variable),position="dodge")+sosp_theme
