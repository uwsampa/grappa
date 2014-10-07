DIR=$SP2B/bin
rm -f sp2bench_1m sp2bench_1m.index
ln -s $DIR/sp2b.100mb.i sp2bench_1m
ln -s $DIR/sp2b.100mb.index sp2bench_1m.index
export NTUPLES=`wc -l $DIR/sp2b.100mb.i | cut -d ' ' -f1`
