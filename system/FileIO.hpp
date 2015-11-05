////////////////////////////////////////////////////////////////////////
// Copyright (c) 2010-2015, University of Washington and Battelle
// Memorial Institute.  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//     * Redistributions of source code must retain the above
//       copyright notice, this list of conditions and the following
//       disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials
//       provided with the distribution.
//     * Neither the name of the University of Washington, Battelle
//       Memorial Institute, or the names of their contributors may be
//       used to endorse or promote products derived from this
//       software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// UNIVERSITY OF WASHINGTON OR BATTELLE MEMORIAL INSTITUTE BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
////////////////////////////////////////////////////////////////////////

#pragma once

#include <unistd.h>
#include <aio.h>
#include <stdint.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

#include <mpi.h>

#include "GlobalAllocator.hpp"
#include "Tasking.hpp"
#include "ParallelLoop.hpp"
#include "Cache.hpp"

#include <sys/stat.h>
#include <iterator>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

DECLARE_uint64( io_blocks_per_node );
DECLARE_uint64( io_blocksize_mb );

namespace Grappa {

#define FNAME_LENGTH 256

namespace impl {
  
  /// (for internal use)
  /// Right now, FileDesc is just a normal POSIX file descriptor.
  /// However, using this level of abstraction in case we want the ability in the
  /// future to open files in a specific way for reading into Grappa.
  typedef int FileDesc;

  inline void array_dir_fname(char * result, const char * dirname, int64_t start, int64_t end) {
    sprintf(result, "%s/block.%ld.%ld.gblk", dirname, (long)start, (long)end);
  }
  inline void array_dir_scan(const fs::path& p, int64_t * start, int64_t * end) {
    //VLOG(1) << p.stem().string();
    static_assert( sizeof(long) == sizeof(int64_t), "long must be 64 bits" );
    sscanf(p.stem().c_str(), "block.%ld.%ld", (long*)start, (long*)end);
  }

/// call in a collective context
void read_unordered_shared( const char * filename, void * local_ptr, size_t local_size );

/// call in a collective context
void write_unordered_shared( const char * filename, void * local_ptr, size_t local_size );

}
  
#ifdef SIGRTMIN
#define AIO_SIGNAL SIGRTMIN+1
#endif

// little helper for iterating over things numerous enough to need to be buffered
#define for_buffered(i, n, start, end, nbuf) \
  for (int64_t i=start, n=nbuf; i<end && (n = std::min(nbuf, end-i)); i+=nbuf)

/// @addtogroup Utility
/// @{

/// Grappa file descriptor.
/// 
/// @param fname       Path/name of either a single file or a directory of files
///                    containing pieces of an array
/// @param isDirectory Whether or not to treat the File as group of blocks
/// @param offset      Point in the file to begin reading (currently not 
///                    supported for directory of files)
struct File {
  char fname[FNAME_LENGTH];
  bool isDirectory;
  size_t offset;
  File(const char * fname, bool asDirectory, size_t offset = 0) {
    _init(fname, asDirectory, offset);
  }
  File(const char * fname, size_t offset = 0) {
    bool asDirectory;
    if (fs::exists(fname)) {
      asDirectory = fs::is_directory(fname);
    } else {
      LOG(ERROR) << "warning: unable to find file/directory: " << fname << ", assuming 'directory'.";
      asDirectory = true;
    }
    _init(fname, asDirectory, offset);
  }
private:
  void _init(const char * fname, bool asDirectory, size_t offset) {
    strncpy(this->fname, fname, FNAME_LENGTH);
    this->isDirectory = asDirectory;
    this->offset = offset;
  }
};

/// Basically a wrapper around a POSIX "struct aiocb" with info for resuming the calling Grappa thread
struct IODescriptor {
  bool complete;
  ConditionVariable cv;
  
  struct aiocb ac;
  IODescriptor * nextCompleted; // for use in completed stack
  
  IODescriptor(int file_desc=0, size_t file_offset = 0, void * buffer = NULL, size_t bufsize = 0) {
    complete = false;
    nextCompleted = NULL;
    ac.aio_reqprio = 0;
    ac.aio_sigevent.sigev_notify = SIGEV_SIGNAL; // notify completion using a signal
#ifdef AIO_SIGNAL
    ac.aio_sigevent.sigev_signo = AIO_SIGNAL;
#endif
    ac.aio_sigevent.sigev_value.sival_ptr = this;
    file(file_desc);
    offset(file_offset);
    buf(buffer, bufsize);
  }
  void file(int file_desc) { ac.aio_fildes = file_desc; }
  void buf(void* buf, size_t nbytes) {
    ac.aio_buf = buf;
    ac.aio_nbytes = nbytes;
  }
  volatile void * buf() { return ac.aio_buf; }
  size_t nbytes() { return ac.aio_nbytes; }
  template<typename T> size_t nelems() { return ac.aio_nbytes/sizeof(T); }
  void offset(size_t of) { ac.aio_offset = of; }
  struct aiocb * desc_ptr() { return &ac; }

  void block_on_read() {
    aio_read(desc_ptr());
    while (!complete) wait(&cv);
  }

  void handle_completion() {
    complete = true;
    signal(&cv);
  }

};

// global stack of completed IO requests
extern IODescriptor * aio_completed_stack;

namespace impl {

  /// Signal handler called when AIO operations complete
  ///
  /// Completed IODescriptor's should wake their caller, but can't do it from
  /// the handler because scheduler could be in inconsistent state. Instead,
  /// add to a queue which will be processed in the polling thread.
  inline void handle_async_io(int sig, siginfo_t * info, void * ucontext) {
    VLOG(3) << "AIO handler called";
    IODescriptor * desc = (IODescriptor*)info->si_value.sival_ptr;

    // add to head of completed list
    // atomic because no other aio completions are allowed to interrupt this one
    desc->nextCompleted = aio_completed_stack;
    aio_completed_stack = desc;
  }

  /// Special fopen so we can be sure to open files correctly for reading asynchronously
  inline FileDesc file_open(const char *const fname, const char *const mode) {
    if (strncmp(mode, "r", FNAME_LENGTH) == 0) {
      int fdesc = open(fname, O_RDONLY);
      if (fdesc == -1) {
        //fprintf(stderr, "Error opening file for read only: %s.\n", fname);
        LOG(FATAL) << "Error opening file for read only: " << fname;
        exit(1);
      }
      return (FileDesc)fdesc;
    } else {
      fprintf(stderr, "File operation not implemented yet.\n");
      return static_cast<FileDesc>(0);
    }
  }

  inline void file_close(FileDesc fdesc) {
    close(fdesc);
  }


  inline void fread_blocking(void * buffer, size_t bufsize, size_t offset, FileDesc file_desc) {
    IODescriptor d(file_desc, offset, buffer, bufsize);
    d.block_on_read();
    CHECK(d.complete);
  }

  template < typename T >
  struct read_array_args {
    char fname[FNAME_LENGTH];
    GlobalAddress<T> base;
    int64_t start, end;
    GlobalAddress<Grappa::CompletionEvent> joiner;
    int64_t file_offset;

    read_array_args(): base(), start(0), end(0), joiner() {}
    read_array_args(const char * fname, GlobalAddress<T> base, int64_t start, int64_t end, GlobalAddress<Grappa::CompletionEvent> joiner): base(base), start(start), end(end), joiner(joiner), file_offset(0) {
      strncpy(this->fname, fname, FNAME_LENGTH);
    }
    static void task(GlobalAddress< read_array_args<T> > args_addr, size_t index, size_t nelem) {
      VLOG(3) << "num_active = " << Grappa::impl::global_scheduler.active_worker_count();

      read_array_args<T> b_args;
      typename Incoherent< read_array_args<T> >::RO args(args_addr, 1, &b_args);
      const read_array_args<T>& a = args[0];

      FileDesc fdesc = file_open(a.fname, "r");

  		T* buf = Grappa::locale_alloc<T>(nelem);
    
      int64_t offset = index - a.start;
      fread_blocking(buf, sizeof(T)*nelem, sizeof(T)*offset+a.file_offset, fdesc);

      { typename Incoherent<T>::WO c(a.base+index, nelem, buf); }
      Grappa::locale_free(buf);

      file_close(fdesc);

      VLOG(2) << "completed read(" << a.start + index << ":" << a.start+index+nelem << ")";
  		Grappa::complete(a.joiner);
    }
  };

  template < typename T >
  void _read_array_file(File& f, GlobalAddress<T> array, size_t nelem) {
    double t = Grappa::walltime();

  	Grappa::call_on_all_cores([]{
  	  Grappa::impl::global_scheduler.allow_active_workers(FLAGS_io_blocks_per_node);
  	});

    const int64_t NBUF = FLAGS_io_blocksize_mb*(1L<<20)/sizeof(T);

    Grappa::CompletionEvent io_joiner;
    auto joiner_addr = make_global(&io_joiner);
  
    read_array_args<T> args(f.fname, array, 0, nelem, joiner_addr);
    args.file_offset = f.offset;
    auto arg_addr = make_global(&args);

    // read array
    for_buffered (i, n, 0, (int64_t)nelem, NBUF) {
      CHECK( i+n <= nelem) << "nelem = " << nelem << ", i+n = " << i+n;
    
      io_joiner.enroll();
      spawn<TaskMode::Unbound>([arg_addr, i, n]{
        read_array_args<T>::task(arg_addr, i, n);
      });
    }
    io_joiner.wait();
  
  	Grappa::call_on_all_cores([]{
  	  Grappa::impl::global_scheduler.allow_active_workers(-1);
  	});

    f.offset += nelem * sizeof(T);
    t = Grappa::walltime() - t;
    VLOG(1) << "read_array_time: " << t;
    VLOG(1) << "read_rate_mbps: " << ((double)nelem * sizeof(T) / (1L<<20)) / t;
  }

  template < typename T >
  void _read_array_dir(File& f, GlobalAddress<T> array, size_t nelem) {
    const char * dirname = f.fname;
    double t = Grappa::walltime();

  	Grappa::call_on_all_cores([]{
  	  Grappa::impl::global_scheduler.allow_active_workers(FLAGS_io_blocks_per_node);
  	});

    size_t nfiles = std::distance(fs::directory_iterator(dirname), fs::directory_iterator());
  
    auto args = locale_alloc< read_array_args<T> >(nfiles);
  
    const int64_t NBUF = FLAGS_io_blocksize_mb*(1L<<20)/sizeof(T);

    Grappa::CompletionEvent io_joiner;
  	auto joiner_addr = make_global(&io_joiner);

    int64_t i = 0;
    fs::directory_iterator d(dirname);
    for (int64_t i = 0; d != fs::directory_iterator(); i++, d++) {
      int64_t start, end;
      array_dir_scan(d->path(), &start, &end);
      //VLOG(1) << "start = " << start << ", end = " << end;
      CHECK( start < end && start < nelem && end <= nelem) << "nelem = " << nelem << ", start = " << start << ", end = " << end;
    
      GlobalAddress< read_array_args<T> > arg_addr = make_global(new (args+i) read_array_args<T>(d->path().string().c_str(), array, start, end, joiner_addr));

      // read array
      for_buffered (i, n, start, end, NBUF) {
        CHECK( i+n <= nelem) << "nelem = " << nelem << ", i+n = " << i+n;
      
        io_joiner.enroll();
        spawn<TaskMode::Unbound>([arg_addr, i, n]{
          read_array_args<T>::task(arg_addr, i, n);
        });
      }
    }
    io_joiner.wait();
  
  	Grappa::call_on_all_cores([]{
  	  Grappa::impl::global_scheduler.allow_active_workers(-1);
  	});

    t = Grappa::walltime() - t;
    VLOG(1) << "read_array_time: " << t;
    VLOG(1) << "read_rate_mbps: " << ((double)nelem * sizeof(T) / (1L<<20)) / t;
    locale_free(args);
  }
  
} // namespace impl

/// Read a file or directory of files into a global array.
template < typename T >
void read_array(File& f, GlobalAddress<T> array, size_t nelem) {
  if (f.isDirectory) {
    impl::_read_array_dir(f, array, nelem);
  } else {
    impl::_read_array_file(f, array, nelem);
  }
}

namespace impl {
  /// Assuming HDFS, so write array to different files in a directory because otherwise we can't write in parallel
  template < typename T >
  void _save_array_dir(const char * dirname, GlobalAddress<T> array, size_t nelems) {
    // make directory with mode 777
    fs::path pdir(dirname);
  
    try {
      fs::create_directories(pdir);
      fs::permissions(pdir, fs::perms::all_all);
    } catch (fs::filesystem_error& e) {
      LOG(ERROR) << "filesystem error: " << e.what();
    }
	
    double t = Grappa::walltime();
    
    size_t namelen = strlen(dirname);
    
    char dir[256]; strcpy(dir, dirname);
    auto g_dir = make_global(dir);
    
    Grappa::on_all_cores([g_dir,namelen,array,nelems]{
      range_t r = blockDist(0, nelems, Grappa::mycore(), Grappa::cores());
      
      char dir[256];
      Incoherent<char>::RO c(g_dir,namelen+1,dir);
      
      char fname[FNAME_LENGTH]; array_dir_fname(fname, &c[0], r.start, r.end);
      std::fstream fo(fname, std::ios::out | std::ios::binary);
      
      VLOG(1) << "saving to " << fname;
      
      const int64_t NBUF = FLAGS_io_blocksize_mb*(1L<<20)/sizeof(T);
      T * buf = Grappa::locale_alloc<T>(NBUF);
      
      for_buffered (i, n, r.start, r.end, NBUF) {
        typename Incoherent<T>::RO c(array+i, n, buf);
        c.block_until_acquired();
        fo.write((char*)buf, sizeof(T)*n);
        VLOG(1) << "wrote " << i << ".." << i+n << " (" << n << ")";
      }
          Grappa::locale_free(buf);
      
      fo.close();
      VLOG(1) << "finished saving array[" << r.start << ":" << r.end << "]";
    });
	
    t = Grappa::walltime() - t;
    VLOG(2) << "save_array_time: " << t;
    VLOG(2) << "save_rate_mbps: " << ((double)nelems * sizeof(T) / (1L<<20)) / t;
  }

  template< typename T >
  void _save_array_file(const char * fname, GlobalAddress<T> array, size_t nelems) {
    double t = Grappa::walltime();

    std::fstream fo(fname, std::ios::out | std::ios::binary);

    const int64_t NBUF = FLAGS_io_blocksize_mb*(1L<<20)/sizeof(T);
  	T * buf = Grappa::locale_alloc<T>(NBUF);
    for_buffered (i, n, 0, (int64_t)nelems, NBUF) {
      typename Incoherent<T>::RO c(array+i, n, buf);
      c.block_until_acquired();
  	  fo.write((char*)buf, sizeof(T)*n);
    }
  	Grappa::locale_free(buf);

    t = Grappa::walltime() - t;
    fo.close();
    VLOG(2) << "save_array_time: " << t;
    VLOG(2) << "save_rate_mbps: " << ((double)nelems * sizeof(T) / (1L<<20)) / t;
  }
} // namespace impl

template< typename T >
void save_array(File& f, bool asDirectory, GlobalAddress<T> array, size_t nelem) {
  if (asDirectory) {
    impl::_save_array_dir(f.fname, array, nelem);
  } else {
    impl::_save_array_file(f.fname, array, nelem);
  }
}


template< typename T >
void write_array_unordered( std::string filename, GlobalAddress<T> array, size_t nelem ) {

  LOG(WARNING) << "Make sure you're writing to a shared filesystem with proper locking!";


  // all this stuff is trying to find a workaround for NFS problems on the sampa cluster
  if( fs::exists( filename ) ) {
    fs::remove( filename );
  }

  // create empty file
  { std::ofstream ofs( filename, std::ios::binary | std::ios::out ); }
  //fs::resize_file( filename, 0 );

  // make sure file exists
  CHECK( fs::exists( filename ) ) << "File not found.";
  CHECK( fs::is_regular_file( filename ) ) << "File is not a regular file.";
  CHECK_EQ( fs::file_size( filename ), 0 );
  
  // helper struct for filename propagation
  struct ArrayWriteHelper {
    std::unique_ptr< char[] > filename;
  } GRAPPA_BLOCK_ALIGNED;
  auto helper = symmetric_global_alloc<ArrayWriteHelper>();

  size_t filename_size = filename.size();
  Core mycore = Grappa::mycore();
  on_all_cores( [=,&filename] {

      // distribute filename across all cores
      helper->filename.reset( new char[filename_size+1] );
      if( Grappa::mycore() == mycore ) {
        strncpy( &helper->filename[0], filename.c_str(), filename.size()+1 );
      }
      MPI_CHECK( MPI_Bcast( &helper->filename[0], filename_size+1, MPI_CHAR, mycore, global_communicator.grappa_comm ) );

      // get local chunk of array
      T * local_ptr = array.localize();
      T * local_end = (array+nelem).localize();
      int64_t local_count = local_end - local_ptr;

      // write from local chunk
      impl::write_unordered_shared( &helper->filename[0], local_ptr, local_count * sizeof(T) );
      
    } );

  // Do this once it's supported
  // symmetric_global_free(helper);
}


template< typename T >
void read_array_unordered( std::string filename, GlobalAddress<T> array, size_t nelem ) {

  // make sure file exists
  CHECK( fs::exists( filename ) ) << "File not found.";
  CHECK( fs::is_regular_file( filename ) ) << "File is not a regular file.";

  size_t file_size = fs::file_size( filename );
  CHECK_EQ( file_size, nelem * sizeof(T) ) << "Array and file are different sizes";
  
  // helper struct for filename propagation
  struct ArrayReadHelper {
    std::unique_ptr< char[] > filename;
  } GRAPPA_BLOCK_ALIGNED;
  auto helper = symmetric_global_alloc<ArrayReadHelper>();

  size_t filename_size = filename.size();
  Core mycore = Grappa::mycore();
  on_all_cores( [=,&filename] {

      // distribute filename across all cores
      helper->filename.reset( new char[filename_size+1] );
      if( Grappa::mycore() == mycore ) {
        strncpy( &helper->filename[0], filename.c_str(), filename.size()+1 );
      }
      MPI_CHECK( MPI_Bcast( &helper->filename[0], filename_size+1, MPI_CHAR, mycore, global_communicator.grappa_comm ) );

      // get local chunk of array
      T * local_ptr = array.localize();
      T * local_end = (array+nelem).localize();
      int64_t local_count = local_end - local_ptr;

      // load into local chunk
      impl::read_unordered_shared( &helper->filename[0], local_ptr, local_count * sizeof(T) );
      
    } );

  // Do this once it's supported
  // symmetric_global_free(helper);
}

} // namespace Grappa

/// @}
