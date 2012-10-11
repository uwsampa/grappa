
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <unistd.h>
#include <aio.h>
#include <stdint.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include "Grappa.hpp"
#include "GlobalAllocator.hpp"
#include "Tasking.hpp"
#include "ForkJoin.hpp"

#include <sys/stat.h>
#include <iterator>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

DECLARE_uint64( io_blocks_per_node );
DECLARE_uint64( io_blocksize_mb );

#define FNAME_LENGTH 256

inline void array_dir_fname(char * result, const char * dirname, int64_t start, int64_t end) {
  sprintf(result, "%s/block.%ld.%ld.gblk", dirname, start, end);
}
inline void array_dir_scan(const fs::path& p, int64_t * start, int64_t * end) {
  //VLOG(1) << p.stem().string();
  sscanf(p.stem().string().c_str(), "block.%ld.%ld", start, end);
}

#define AIO_SIGNAL SIGRTMIN+1

// little helper for iterating over things numerous enough to need to be buffered
#define for_buffered(i, n, start, end, nbuf) \
  for (size_t i=start, n=nbuf; i<end && (n = MIN(nbuf, end-i)); i+=nbuf)

/// Grappa file descriptor.
/// 
/// @param fname       Path/name of either a single file or a directory of files
///                    containing pieces of an array
/// @param isDirectory Whether or not to treat the GrappaFile as group of blocks
/// @param offset      Point in the file to begin reading (currently not 
///                    supported for directory of files)
struct GrappaFile {
  char fname[FNAME_LENGTH];
  bool isDirectory;
  size_t offset;
  GrappaFile(const char * fname, bool asDirectory, size_t offset = 0) {
    _init(fname, asDirectory, offset);
  }
  GrappaFile(const char * fname, size_t offset = 0) {
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

/// (for internal use)
/// Right now, GrappaFileDesc is just a normal POSIX file descriptor.
/// However, using this level of abstraction in case we want the ability in the
/// future to open files in a specific way for reading into Grappa.
typedef int GrappaFileDesc;

/// Basically a wrapper around a POSIX "struct aiocb" with info for resuming the calling Grappa thread
struct IODescriptor {
  bool complete;
  Thread * waiter;
  struct aiocb ac;
  IODescriptor * nextCompleted; // for use in completed stack
  
  IODescriptor(int file_desc=0, size_t file_offset = 0, void * buffer = NULL, size_t bufsize = 0) {
    complete = false;
    waiter = NULL;
    nextCompleted = NULL;
    ac.aio_reqprio = 0;
    ac.aio_sigevent.sigev_notify = SIGEV_SIGNAL; // notify completion using a signal
    ac.aio_sigevent.sigev_signo = AIO_SIGNAL;
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
    waiter = CURRENT_THREAD;
    aio_read(desc_ptr());
    if (!complete) Grappa_suspend();
  }

  void handle_completion() {
    complete = true;
    if (waiter != NULL) {
      Thread * waking = waiter;
      waiter = NULL;
      Grappa_wake(waking);
    }
  }

};

// global stack of completed IO requests
extern IODescriptor * aio_completed_stack;

/// Signal handler called when AIO operations complete
///
/// Completed IODescriptor's should wake their caller, but can't do it from
/// the handler because scheduler could be in inconsistent state. Instead,
/// add to a queue which will be processed in the polling thread.
inline void Grappa_handle_aio(int sig, siginfo_t * info, void * ucontext) {
  VLOG(3) << "AIO handler called";
  IODescriptor * desc = (IODescriptor*)info->si_value.sival_ptr;

  // add to head of completed list
  // atomic because no other aio completions are allowed to interrupt this one
  desc->nextCompleted = aio_completed_stack;
  aio_completed_stack = desc;
}

/// Special fopen so we can be sure to open files correctly for reading asynchronously
inline GrappaFileDesc Grappa_fopen(const char *const fname, const char *const mode) {
  if (strncmp(mode, "r", FNAME_LENGTH) == 0) {
    int fdesc = open(fname, O_RDONLY);
    if (fdesc == -1) {
      fprintf(stderr, "Error opening file for read only: %s.\n", fname);
      exit(1);
    }
    return (GrappaFileDesc)fdesc;
  } else {
    fprintf(stderr, "File operation not implemented yet.\n");
  }
}

inline void Grappa_fclose(GrappaFileDesc fdesc) {
  close(fdesc);
}

inline void Grappa_fread_suspending(void * buffer, size_t bufsize, size_t offset, GrappaFileDesc file_desc) {
  IODescriptor d(file_desc, offset, buffer, bufsize);
  d.block_on_read();
  CHECK(d.complete);
}

template < typename T >
struct read_array_args {
  char fname[FNAME_LENGTH];
  GlobalAddress<T> base;
  int64_t start, end;
  GlobalAddress<LocalTaskJoiner> joiner;
  int64_t file_offset;

  read_array_args(): base(), start(0), end(0), joiner() {}
  read_array_args(const char * fname, GlobalAddress<T> base, int64_t start, int64_t end, GlobalAddress<LocalTaskJoiner> joiner): base(base), start(start), end(end), joiner(joiner), file_offset(0) {
    strncpy(this->fname, fname, FNAME_LENGTH);
  }
  static void task(GlobalAddress< read_array_args<T> > args_addr, size_t index, size_t nelem) {
    VLOG(3) << "num_active = " << global_scheduler.active_worker_count();

    read_array_args<T> b_args;
    typename Incoherent< read_array_args<T> >::RO args(args_addr, 1, &b_args);
    const read_array_args<T>& a = args[0];

    GrappaFileDesc fdesc = Grappa_fopen(a.fname, "r");

    T* buf = new T[nelem];
    
    int64_t offset = a.file_offset + index - a.start;
    Grappa_fread_suspending(buf, sizeof(T)*nelem, sizeof(T)*offset, fdesc);
    //for (int i=0; i<nelem; i++) {
      //VLOG(1) << buf[i];
    //}

    { typename Incoherent<T>::WO c(a.base+index, nelem, buf); }
    delete [] buf;

    Grappa_fclose(fdesc);

    VLOG(2) << "completed read(" << a.start + index << ":" << a.start+index+nelem << ")";
    LocalTaskJoiner::remoteSignal(a.joiner);
  }
};

LOOP_FUNCTOR( set_allow_active, nid, ((int64_t,n)) ) {
  // limit number of workers allowed to be active
  global_scheduler.allow_active_workers(n);
}

template < typename T >
void _read_array_file(const GrappaFile& f, GlobalAddress<T> array, size_t nelem) {
  double t = Grappa_walltime();

  { set_allow_active f(FLAGS_io_blocks_per_node); fork_join_custom(&f); }

  const size_t NBUF = FLAGS_io_blocksize_mb*(1L<<20)/sizeof(T);

  LocalTaskJoiner io_joiner;
  GlobalAddress<LocalTaskJoiner> joiner_addr = make_global(&io_joiner);
  
  read_array_args<T> args(f.fname, array, 0, nelem, joiner_addr);
  args.file_offset = f.offset;
  GlobalAddress< read_array_args<T> > arg_addr = make_global(&args);

  // read array
  for_buffered (i, n, 0, nelem, NBUF) {
    CHECK( i+n <= nelem) << "nelem = " << nelem << ", i+n = " << i+n;
    
    io_joiner.registerTask();
    Grappa_publicTask(&read_array_args<T>::task, arg_addr, i, n);
  }
  io_joiner.wait();
  
  { set_allow_active f(-1); fork_join_custom(&f); }

  t = Grappa_walltime() - t;
  VLOG(1) << "read_array_time: " << t;
  VLOG(1) << "read_rate_mbps: " << ((double)nelem * sizeof(T) / (1L<<20)) / t;
}

template < typename T >
void _read_array_dir(const GrappaFile& f, GlobalAddress<T> array, size_t nelem) {
  const char * dirname = f.fname;
  double t = Grappa_walltime();

  { set_allow_active f(FLAGS_io_blocks_per_node); fork_join_custom(&f); }

  size_t nfiles = std::distance(fs::directory_iterator(dirname), fs::directory_iterator());
  
  read_array_args<T> args[nfiles];
  
  const size_t NBUF = FLAGS_io_blocksize_mb*(1L<<20)/sizeof(T);

  LocalTaskJoiner io_joiner;
  GlobalAddress<LocalTaskJoiner> joiner_addr = make_global(&io_joiner);

  size_t i = 0;
  fs::directory_iterator d(dirname);
  for (size_t i = 0; d != fs::directory_iterator(); i++, d++) {
    int64_t start, end;
    array_dir_scan(d->path(), &start, &end);
    //VLOG(1) << "start = " << start << ", end = " << end;
    CHECK( start < end && start < nelem && end <= nelem) << "nelem = " << nelem << ", start = " << start << ", end = " << end;
    
    GlobalAddress< read_array_args<T> > arg_addr = make_global(new (args+i) read_array_args<T>(d->path().string().c_str(), array, start, end, joiner_addr));

    // read array
    for_buffered (i, n, start, end, NBUF) {
      CHECK( i+n <= nelem) << "nelem = " << nelem << ", i+n = " << i+n;
      
      io_joiner.registerTask();
      Grappa_publicTask(&read_array_args<T>::task, arg_addr, i, n);
    }
  }
  io_joiner.wait();
  
  { set_allow_active f(-1); fork_join_custom(&f); }

  t = Grappa_walltime() - t;
  VLOG(1) << "read_array_time: " << t;
  VLOG(1) << "read_rate_mbps: " << ((double)nelem * sizeof(T) / (1L<<20)) / t;
}

/// Read a file or directory of files into a global array.
template < typename T >
void Grappa_read_array(GrappaFile& f, GlobalAddress<T> array, size_t nelem) {
  if (f.isDirectory) {
    _read_array_dir(f.fname, array, nelem);
  } else {
    _read_array_file(f.fname, array, nelem);
  }
}

template < typename T >
struct save_array_func : ForkJoinIteration {
    GlobalAddress<T> array; size_t nelems; char dirname[FNAME_LENGTH];
  save_array_func() {}
  save_array_func( const char dirname[FNAME_LENGTH], GlobalAddress<T> array, size_t nelems):
    array(array), nelems(nelems) { memcpy(this->dirname, dirname, FNAME_LENGTH); }
  void operator()(int64_t nid) const {
    range_t r = blockDist(0, nelems, Grappa_mynode(), Grappa_nodes());
    char fname[FNAME_LENGTH]; array_dir_fname(fname, dirname, r.start, r.end);
    std::fstream fo(fname, std::ios::out | std::ios::binary);
    
    //const size_t NBUF = BUFSIZE/sizeof(T); 
    const size_t NBUF = FLAGS_io_blocksize_mb*(1L<<20)/sizeof(T);
    T * buf = new T[NBUF];
    for_buffered (i, n, r.start, r.end, NBUF) {
      typename Incoherent<T>::RO c(array+i, n, buf);
      c.block_until_acquired();
      fo.write((char*)buf, sizeof(T)*n);
      //VLOG(1) << "wrote " << n << " bytes";
    }
    delete [] buf;

    fo.close();
    VLOG(1) << "finished saving array[" << r.start << ":" << r.end << "]";
  }
};

/// Assuming HDFS, so write array to different files in a directory because otherwise we can't write in parallel
template < typename T >
void _save_array_dir(const char * dirname, GlobalAddress<T> array, size_t nelems) {
  // make directory with mode 777
  //fs::create_directory((const char *)dirname);
  if ( mkdir((const char *)dirname, 0777) != 0 ) {
    switch (errno) { // error occurred
      case EEXIST: LOG(INFO) << "files already exist, skipping write..."; return;
      default: fprintf(stderr, "Error with `mkdir`!\n"); break;
    }
  }

  double t = Grappa_walltime();

  { save_array_func<T> f(dirname, array, nelems); fork_join_custom(&f); }
  
  t = Grappa_walltime() - t;
  LOG(INFO) << "save_array_time: " << t;
  LOG(INFO) << "save_rate_mbps: " << ((double)nelems * sizeof(T) / (1L<<20)) / t;
}

template< typename T >
void _save_array_file(const char * fname, GlobalAddress<T> array, size_t nelems) {
  double t = Grappa_walltime();

  std::fstream fo(fname, std::ios::out | std::ios::binary);

  //const size_t NBUF = BUFSIZE/sizeof(T); 
  const size_t NBUF = FLAGS_io_blocksize_mb*(1L<<20)/sizeof(T);
  T * buf = new T[NBUF];
  for_buffered (i, n, 0, nelems, NBUF) {
    typename Incoherent<T>::RO c(array+i, n, buf);
    c.block_until_acquired();
  fo.write((char*)buf, sizeof(T)*n);
  }
  delete [] buf;

  t = Grappa_walltime() - t;
  fo.close();
  LOG(INFO) << "save_array_time: " << t;
  LOG(INFO) << "save_rate_mbps: " << ((double)nelems * sizeof(T) / (1L<<20)) / t;
}

template< typename T >
void Grappa_save_array(GrappaFile& f, bool asDirectory, GlobalAddress<T> array, size_t nelem) {
  if (asDirectory) {
    _save_array_dir(f.fname, array, nelem);
  } else {
    _save_array_file(f.fname, array, nelem);
  }
}

