
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <boost/test/unit_test.hpp>
#include "Grappa.hpp"
#include "Tasking.hpp"
#include "FileIO.hpp"

BOOST_AUTO_TEST_SUITE( FileIO_tests );

#define read Grappa_delegate_read_word

static const size_t N = (1L<<10);
static const size_t NN = (1L<<10);
static const size_t BUFSIZE = (1L<<8);

LocalTaskJoiner ljoin;

GlobalAddress<int64_t> array;

void task_read_chunk(size_t offset_elem, size_t nelem, int64_t fdesc_packed) {
  GrappaFileDesc fdesc = (GrappaFileDesc)fdesc_packed;
  int64_t * buf = new int64_t[nelem];

  Grappa_fread_suspending(buf, nelem*sizeof(int64_t), offset_elem*sizeof(int64_t), fdesc);

  { 
    Incoherent<int64_t>::WO c(array+offset_elem, nelem, buf);
  }

  delete[] buf;
  ljoin.signal();
}

void test_single_read() {
  // create test file to read from
  char fname[256];
  sprintf(fname, "fileio_tests_seq.%ld.bin", N);
  FILE * fout = fopen(fname, "w");

  int64_t * loc_array = new int64_t[N];
  for (int64_t i=0; i<N; i++) { loc_array[i] = i; }
  fwrite(loc_array, N, sizeof(int64_t), fout);
  delete[] loc_array;
  fclose(fout);

  // do file io using asynchronous POSIX/suspending IO
  array = Grappa_typed_malloc<int64_t>(N);
  Grappa_memset_local(array, (int64_t)0, N);

  const size_t nbuf = BUFSIZE/sizeof(int64_t);

  GrappaFileDesc fdesc = Grappa_fopen(fname, "r");
  const size_t ntasks = N/nbuf;

  ljoin.reset();

  size_t offset = 0;
  for_buffered(i, n, 0, N, nbuf) {
    ljoin.registerTask();
    Grappa_privateTask(task_read_chunk, i, n, (int64_t)fdesc);
  }
  ljoin.wait();

  for (int64_t i=0; i<N; i++) {
    CHECK(Grappa_delegate_read_word(array+i) == i);
  }

  // clean up
  Grappa_free(array);
  if (remove(fname)) { fprintf(stderr, "Error removing file: %s.\n", fname); }
}

LOOP_FUNCTION( func_barrier, nid ) {
  Grappa_barrier_suspending();
}

void test_read_save_array(bool asDirectory) {
  FLAGS_io_blocksize_mb = 1;

  // create test file to read from
  char fname[256];
  sprintf(fname, "/scratch/hdfs/tmp/fileio_tests_seq.%ld.bin", NN);
  GrappaFile f(fname, asDirectory);

  // do file io using asynchronous POSIX/suspending IO
  array = Grappa_typed_malloc<int64_t>(NN);

  const size_t NBUF = FLAGS_io_blocksize_mb*(1L<<20)/sizeof(int64_t);

  int64_t * buf = new int64_t[NBUF];
  for_buffered(i, n, 0, NN, NBUF) {
		for (int64_t j=0; j<n; j++) {
      buf[j] = i+j;
    }
    Incoherent<int64_t>::WO c(array+i, n, buf);
  }
  Grappa_save_array(f, asDirectory, array, NN);

  Grappa_memset_local(array, (int64_t)0, NN);

  sleep(5); // having it wait here helps it crash less due to inconsistent FS state

  Grappa_read_array(f, array, NN);

  for_buffered(i, n, 0, NN, NBUF) {
    VLOG(1) << "i = " << i << ", n = " << n;
    Incoherent<int64_t>::RO c(array+i, n, buf);
		for (int64_t j=0; j<n; j++) {
      CHECK(c[j] == i+j) << "i = " << i << ", j = " << j << ", array[i+j] = " << c[j];
    }
  }

  // clean up
  Grappa_free(array);
  if (fs::exists(fname)) { fs::remove_all(fname); }
}

void user_main( void * ignore ) {
  test_single_read();
  //test_read_save_array(false);
  sleep(1);
  test_read_save_array(true);
}


BOOST_AUTO_TEST_CASE( test1 ) {

  Grappa_init( &(boost::unit_test::framework::master_test_suite().argc),
                &(boost::unit_test::framework::master_test_suite().argv) );

  Grappa_activate();

  Grappa_run_user_main( &user_main, (void*)NULL );

  Grappa_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();

