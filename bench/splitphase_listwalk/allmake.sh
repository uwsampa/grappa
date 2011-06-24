THIS_DIR=`pwd`
BASE_DIR=$THIS_DIR/../..
Q_DIR=$BASE_DIR/queues
G_DIR=$BASE_DIR/global_memory

cd $Q_DIR
make clean libcorequeue.a

cd $G_DIR
make clean libsplitphase.a libbprintf.a

cd $THIS_DIR
make clean
make
