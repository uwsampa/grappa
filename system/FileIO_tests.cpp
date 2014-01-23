////////////////////////////////////////////////////////////////////////
// This file is part of Grappa, a system for scaling irregular
// applications on commodity clusters. 

// Copyright (C) 2010-2014 University of Washington and Battelle
// Memorial Institute. University of Washington authorizes use of this
// Grappa software.

// Grappa is free software: you can redistribute it and/or modify it
// under the terms of the Affero General Public License as published
// by Affero, Inc., either version 1 of the License, or (at your
// option) any later version.

// Grappa is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// Affero General Public License for more details.

// You should have received a copy of the Affero General Public
// License along with this program. If not, you may obtain one from
// http://www.affero.org/oagpl.html.
////////////////////////////////////////////////////////////////////////

#include <boost/test/unit_test.hpp>
#include "Grappa.hpp"
#include "Tasking.hpp"
#include "FileIO.hpp"
#include "Array.hpp"
#include "Cache.hpp"

using namespace Grappa;

BOOST_AUTO_TEST_SUITE( FileIO_tests );

static const size_t N = (1L<<10);
static const size_t NN = (1L<<10);
static const size_t BUFSIZE = (1L<<8);

GlobalAddress<int64_t> array;

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
  array = global_alloc<int64_t>(N);
  Grappa::memset(array, 0, N);
  
  const size_t nbuf = BUFSIZE/sizeof(int64_t);

  impl::FileDesc fdesc = file_open(fname, "r");
  const size_t ntasks = N/nbuf;

  CompletionEvent ce;

  size_t offset = 0;
  
  for_buffered(i, n, 0, N, nbuf) {
    spawn(&ce, [=]{
      auto f = static_cast<impl::FileDesc>(fdesc);
      int64_t * buf = locale_alloc<int64_t>(n);
      
      fread_blocking(buf, n*sizeof(int64_t), i*sizeof(int64_t), f);
      
      { Incoherent<int64_t>::WO c(array+i, n, buf); }
      
      locale_free(buf);
    });
  }
  
  ce.wait();

  for (int64_t i=0; i<N; i++) {
    CHECK(delegate::read(array+i) == i);
  }

  // clean up
  global_free(array);
  if (remove(fname)) { fprintf(stderr, "Error removing file: %s.\n", fname); }
}


void test_read_save_array(bool asDirectory) {
  FLAGS_io_blocksize_mb = 1;

  // create test file to read from
  char fname[256];
  if( boost::filesystem::exists( "/scratch/hdfs" ) ) {
    // probably the Sampa cluster
    sprintf(fname, "/scratch/hdfs/fileio_tests_seq.%ld.bin", NN);
  } else { 
    // probably not the Sampa cluster.
    // Assume current directory is shared across the cluster.
    sprintf(fname, "./fileio_tests_seq.%ld.bin", NN);
  }
  Grappa::File f(fname, asDirectory);

  // do file io using asynchronous POSIX/suspending IO
  array = global_alloc<int64_t>(NN);
  
  const size_t NBUF = FLAGS_io_blocksize_mb*(1L<<20)/sizeof(int64_t);

  int64_t * buf = locale_alloc<int64_t>(NBUF);
  
  forall(array, NN, [](int64_t i, int64_t& e){ e = i; });

  save_array(f, asDirectory, array, NN);
  
  Grappa::memset(array, 0, NN);

  sleep(5); // having it wait here helps it crash less due to inconsistent FS state

  Grappa::read_array(f, array, NN);
  
  forall(array, NN, [](int64_t i, int64_t& e){
    // CHECK_EQ(e, i);
    BOOST_CHECK_EQUAL(e, i);
  });
  
  // clean up
  Grappa::global_free(array);
  locale_free(buf);
  if (fs::exists(fname)) { fs::remove_all(fname); }
}

BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa::init( GRAPPA_TEST_ARGS );
  Grappa::run([]{
    test_single_read();
    
    // LOG(INFO) << "testing file read/write";
    // test_read_save_array(false);
    
    sleep(1);
    
    LOG(INFO) << "testing dir read/write";
    test_read_save_array(true);
  });
  Grappa::finalize();
}

BOOST_AUTO_TEST_SUITE_END();

