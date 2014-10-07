DIR=/sampa/home/bdmyers/escience/datalogcompiler/c_test_environment
rm -f sp2bench_1m sp2bench_1m.index
ln -s $DIR/sp2b.100t.i sp2bench_1m
ln -s $DIR/sp2b.100t.index sp2bench_1m.index
export NTUPLES=`wc -l $DIR/sp2b.100t.i | cut -d ' ' -f1`
