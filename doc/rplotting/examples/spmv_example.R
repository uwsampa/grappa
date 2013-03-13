spmv.data<-read.csv("spmv.csv")
attach(spmv.data)

par(mfrow=c(1,1))
plot(nnode, make_graph_time)

counts <- table(spmv.data$nnode,spmv.data$nnz_factor)
mean.make_graph_time = aggregate(make_graph_time, by=list(nnz_factor=nnz_factor), FUN=mean)

barplot(as.matrix(mean.make_graph_time), col=c("darkblue","red"),legend=rownames(mean.make_graph_time))

###############################
# Example: stacked bar plot
###############################
# We want to plot the total time to generate the graph,
# showing the proportions make_graph_time and tuple_to_csr_time

# select the cell we care about
selected <- spmv.data[(nnode==2||nnode==12)&logN==21&nnz_factor==16,]

#avg over nnode since nnode==2 had multiple entries 
#be careful about this. In tableau we would like to export all factors (like trial)
selected.aggr <- aggregate(selected, by=list(nnode=selected$nnode), FUN=mean)

# turn into matrix, which is supported data type of barplot
m <- as.matrix(selected.aggr)

# transpose, since we need the data for each stacked bar to be in a column,
# and values to be in different rows
m.t <- t(m)

# select only the two rows for make_graph_time and tuple_to_csr_time
m.t.sub <- m.t[c("make_graph_time","tuple_to_csr_time"),]

# plot, using the rownames as legend, and nnode to label the bars
barplot(m.t.sub, col=c("darkblue","red"),legend=rownames(m.t.sub),names.arg=m.t["nnode",],xlab="nnodes",ylab="time")

###############################
# Now as a proportions bar plot
###############################

# find total time for each instance
selected.aggr[,"total_time"] = selected.aggr$make_graph_time + selected.aggr$tuple_to_csr_time

#as before...
m <- as.matrix(selected.aggr)
m.t <- t(m)

# normalize
m.t["make_graph_time",] = m.t["make_graph_time",]/m.t["total_time",]
m.t["tuple_to_csr_time",] = m.t["tuple_to_csr_time",]/m.t["total_time",]

#as before...
m.t.sub <- m.t[c("make_graph_time","tuple_to_csr_time"),]
barplot(m.t.sub, col=c("darkblue","red"),legend=rownames(m.t.sub),names.arg=m.t["nnode",],xlab="nnodes",ylab="frac time")
