
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

/// Grappa implementation of a 1D FFT beginning and ending in a single Grappa global array
/// Benchmark parameters:
///   --scale,-s  log2(nelem)

#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <sys/stat.h>
#include <math.h>
#include <complex>
using std::complex;

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>

#include <Grappa.hpp>
#include <GlobalAllocator.hpp>
#include <Cache.hpp>
#include <ForkJoin.hpp>
#include <GlobalTaskJoiner.hpp>
#include <Collective.hpp>
#include <Delegate.hpp>

namespace grappa {
  inline int64_t read(GlobalAddress<int64_t> addr) { return Grappa_delegate_read_word(addr); }
  inline void write(GlobalAddress<int64_t> addr, int64_t val) { return Grappa_delegate_write_word(addr, val); }
  inline bool cmp_swap(GlobalAddress<int64_t> address, int64_t cmpval, int64_t newval) {
    return Grappa_delegate_compare_and_swap_word(address, cmpval, newval);
  }
}

inline void set_random(complex<double> * v) {
  typedef boost::mt19937_64 engine_t;
  typedef boost::uniform_real<> dist_t;
  typedef boost::variate_generator<engine_t&,dist_t> gen_t;
  
  // continue using same generator with multiple calls (to not repeat numbers)
  // but start at different seed on each node so we don't get overlap
  static engine_t engine(12345L*Grappa_mynode());
  static gen_t gen(engine, dist_t(-1.0, 1.0));
  
  new (v) complex<double>(gen(), gen());
}

// little helper for iterating over things numerous enough to need to be buffered
#define for_buffered(i, n, start, end, nbuf) \
  for (size_t i=start, n=nbuf; i<end && (n = MIN(nbuf, end-i)); i+=nbuf)

static void printHelp(const char * exe);
static void parseOptions(int argc, char ** argv);

template< typename T >
inline void print_array(const char * name, std::vector<T> v) {
  std::stringstream ss; ss << name << ": [";
  for (size_t i=0; i<v.size(); i++) ss << " " << v[i];
  ss << " ]"; VLOG(1) << ss.str();
}
template< typename T >
inline void print_array(const char * name, GlobalAddress<T> base, size_t nelem) {
  std::stringstream ss; ss << name << ": [";
  //for (size_t i=0; i<nelem; i++) ss << " " << grappa::read(base+i);
  T _buf;
  for (size_t i=0; i<nelem; i++) {
    typename Incoherent<T>::RO c(base+i, 1, &_buf);
    ss << " " << *c;
  }
  ss << " ]"; VLOG(1) << ss.str();
}


static int64_t scale, nelem;
static bool do_verify;

///////////////
// Prototypes
///////////////
void fft(GlobalAddress< complex<double> > v, int64_t nelem);
void verify(complex<double> * sig, complex<double> * result, size_t nelem);

void user_main(void* ignore) {
  Grappa_reset_stats();
  double t, rand_time, fft_time;


  LOG(INFO) << "### FFT Benchmark ###";
  LOG(INFO) << "nelem = (1 << " << scale << ") = " << nelem << " (" << ((double)nelem)*sizeof(complex<double>)/(1L<<30) << " GB)";
  
  LOG(INFO) << "block_size = " << block_size;

  GlobalAddress< complex<double> > sig = Grappa_typed_malloc< complex<double> >(nelem);

    t = Grappa_walltime();

  // fill vector with random 64-bit integers
  forall_local<complex<double>,set_random>(sig, nelem);

    rand_time = Grappa_walltime() - t;
    LOG(INFO) << "fill_random_time: " << rand_time;

  //print_array("signal", sig, nelem);

    //print_sig("generated sig", sig, nelem);
  complex<double> * sig_local;
  if (do_verify) {
    sig_local = new complex<double>[nelem];
    Incoherent< complex<double> >::RO c(sig, nelem, sig_local);
    c.block_until_acquired();
  }

    t = Grappa_walltime();

  fft(sig, scale);

    fft_time = Grappa_walltime() - t;
    LOG(INFO) << "fft_time: " << fft_time;
    LOG(INFO) << "fft_performance: " << (double)(nelem * scale) / fft_time;

  Grappa_merge_and_dump_stats(std::cerr);

  if (do_verify) {
    complex<double> * result = new complex<double>[nelem];
    Incoherent< complex<double> >::RO c(sig, nelem, result);
    c.block_until_acquired();
    
    verify(sig_local, result, nelem);
    delete [] result;
    delete [] sig_local;
  }

}

int main(int argc, char* argv[]) {
  Grappa_init(&argc, &argv);
  Grappa_activate();
  
  parseOptions(argc, argv);
  
  Grappa_run_user_main(&user_main, (void*)NULL);
  
  Grappa_finish(0);
  return 0;
}

static void printHelp(const char * exe) {
  printf("Usage: %s [options]\nOptions:\n", exe);
  printf("  --help,h   Prints this help message displaying command-line options\n");
  printf("  --scale,s  Number of keys to be sorted: 2^SCALE keys.\n");
  printf("  --verify,e  Fully verify FFT result with what FFTW generates.\n");
  exit(0);
}

static void parseOptions(int argc, char ** argv) {
  struct option long_opts[] = {
    {"help", no_argument, 0, 'h'},
    {"scale", required_argument, 0, 's'},
    {"verify", no_argument, 0, 'e'},
  };
  
  // defaults
  scale = 8;
  do_verify = false;

  int c = 0;
  while (c != -1) {
    int option_index = 0;
    c = getopt_long(argc, argv, "hs:e", long_opts, &option_index);
    switch (c) {
      case 'h':
        printHelp(argv[0]);
        exit(0);
      case 's':
        scale = atoi(optarg);
        break;
      case 'e':
        do_verify = true;
        break;
    }
  }
  nelem = 1L << scale;
}

