#!/usr/bin/env Rscript
library(sqldf)
library(ggplot2)

sosp <- 

df <- sqldf( "select * from gups", dbname="~/gups-ec2.db" )

s <- subset( df, experiment_id==1347440606 & flushticks==2000000 & pollticks==20000,
   select=c(ppn, nworkers, flushticks, pollticks, updates_per_s) )

ggplot( s, aes( x=ppn, y=updates_per_s, group=nworkers, fill=nworkers ) ) +
	geom_bar(position="dodge", stat="identity")

ggsave("plots.pdf")
