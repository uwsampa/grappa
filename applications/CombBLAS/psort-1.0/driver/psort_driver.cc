/*

Copyright (c) 2009, David Cheng, Viral B. Shah.

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

#include <mpi.h>
#include <iostream>
#include <cstdlib>
#include <cmath>
#include "MersenneTwister.h"

#include "psort.h"
#include "psort_samplesort.h"

static void check_result (double *work, int mysize) {
  int nproc, rank;
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  MPI_Comm_size (MPI_COMM_WORLD, &nproc);

  /*
  for (long p=0; p<nproc; ++p) {
    MPI_Barrier (MPI_COMM_WORLD);
    if (p == rank) {
      for (long i=0; i<mysize; ++i) std::cerr << rank << ":" << work[i] << std::endl;
    }
    MPI_Barrier (MPI_COMM_WORLD);
  }  
  */

  if (rank == 0) std::cout << "Verifying result... " << std::endl;
  if (vpsort::is_sorted (work, work + mysize, 
			std::less<double>(), 
			MPI_COMM_WORLD)) {
    if (rank == 0) std::cout << "Correctly sorted" << std::endl;
  } else {
    if (rank == 0) std::cout << "NOT correctly sorted" << std::endl;
  }
}

void genData (char type, int rank, double *work, int mysize) 
{

  MTRand mtrand (rank);

  switch (type) {

  case 'r':
    /* 1:n */
    for (long i=0; i<mysize; ++i) work[i] = rank * mysize + i;
    break;

  case 'u':
    /* Uniformly random */
    for (long i=0; i<mysize; ++i) work[i] = mtrand ();
    break;

  case 'n':
    /* Normal random */
    for (long i=0; i<mysize; ++i) work[i] = mtrand.randNorm (0, 100);
    break;

  case 'c':
    /* Cauchy */
    for (long i=0; i<mysize; ++i) work[i] = mtrand.randNorm (0, 1) / 
				    mtrand.randNorm (0, 1);
    break;

  default:
    std::cerr << "Invalid data generator" << std::endl;
    exit (1);
    break;
  }

  //for (long i=0; i<mysize; ++i) std::cout << work[i] << std::endl;
}

int main (int argc, char *argv[]) 
{
  // Initialize
  int nproc, rank;
  MPI_Init (&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nproc);

  long size;
  char splitter, merger, datagen;

  if (argc == 5) {
    size = atol (argv[1]);
    datagen = *argv[2];
    splitter = *argv[3];
    merger = *argv[4];
  }
  else {
    if (rank == 0) {
      std::cout << "Usage: " << argv[0] << " psize datagen splitter merger" << std::endl;
      std::cout << "datagen: r - 1:n, u - uniform random, n - normal random, c - cauchy random" << std::endl;
      std::cout << "splitter: l - sample sort" << std::endl;
      std::cout << "splitter: m - median, s - sample" << std::endl;
      std::cout << "merger: t - tree, o - out-of-place tree, f - funnel" << std::endl;
    }
    exit(0);
  }

  if (rank == 0) std::cout << "PSIZE: " << size << ", NPROC: " << nproc << std::endl;

  long mysize = size / (long) nproc;
  double *work = new double[mysize];
  long dist[nproc];

  for (int i=0; i<nproc; ++i) dist[i] = mysize;

  genData (datagen, rank, work, mysize);

  // First do a dry run to initialize all page tables 
  // Also demonstrate the use of exception handling with psort

  try {
    vpsort::parallel_sort (work, work + mysize,  dist, MPI_COMM_WORLD); 
    //vpsort::parallel_samplesort (work, work + mysize, dist, MPI_COMM_WORLD);
  }
  catch (...) {
    std::cerr << "Exception occured - most likely overflow in psort_alltoall.h" << std::endl;

    delete [] work;

    MPI_Finalize ();
    return 0;
  }
    
  // Now really start

  // Possible sequential sorts
  vpsort::STLSort stl_sort;    
  //vpsort::STLStableSort stl_stable_sort;

  // Possible splitting strategies
  vpsort::MedianSplit median_split;
  vpsort::SampleSplit sample_split;

  // Possible merging strategies
  //vpsort::FlatMerge flat_merge;
  vpsort::TreeMerge tree_merge;
  vpsort::OOPTreeMerge oop_tree_merge;
  vpsort::FunnelMerge2 funnel_merge2;
  //vpsort::FunnelMerge4 funnel_merge4;

  genData (datagen, rank, work, mysize);

  if (splitter == 's') {
    if (merger == 'o') {
      vpsort::parallel_sort (work, work + mysize, std::less<double>(), dist, 
			    stl_sort, sample_split, oop_tree_merge,
			    MPI_COMM_WORLD); 
      vpsort::print_perf_data (dist, 
			      stl_sort, sample_split, oop_tree_merge,
			      MPI_COMM_WORLD);
    } else if (merger == 't') {
      vpsort::parallel_sort (work, work + mysize, std::less<double>(), dist, 
			    stl_sort, sample_split, tree_merge,
			    MPI_COMM_WORLD); 
      vpsort::print_perf_data (dist, 
			      stl_sort, sample_split, tree_merge,
			      MPI_COMM_WORLD);
    } else if (merger == 'f') {
      vpsort::parallel_sort (work, work + mysize, std::less<double>(), dist, 
			    stl_sort, sample_split, funnel_merge2,
			    MPI_COMM_WORLD); 
      vpsort::print_perf_data (dist, 
			      stl_sort, sample_split, funnel_merge2,
			      MPI_COMM_WORLD);
    } else {
      if (rank == 0) std::cerr << "Invalid arguments" << std::endl;
    }
  } else if (splitter == 'm') {
    if (merger == 'o') {
      vpsort::parallel_sort (work, work + mysize, std::less<double>(), dist, 
			    stl_sort, median_split, oop_tree_merge,
			    MPI_COMM_WORLD); 
      vpsort::print_perf_data (dist, 
			      stl_sort, median_split, oop_tree_merge,
			      MPI_COMM_WORLD);
    } else if (merger == 't') {
      vpsort::parallel_sort (work, work + mysize, std::less<double>(), dist, 
			    stl_sort, median_split, tree_merge,
			    MPI_COMM_WORLD); 
      vpsort::print_perf_data (dist, 
			      stl_sort, median_split, tree_merge,
			      MPI_COMM_WORLD);
    } else if (merger == 'f') {
      vpsort::parallel_sort (work, work + mysize, std::less<double>(), dist, 
			    stl_sort, median_split, funnel_merge2,
			    MPI_COMM_WORLD); 
      vpsort::print_perf_data (dist, 
			      stl_sort, median_split, funnel_merge2,
			      MPI_COMM_WORLD);
    } else {
      if (rank == 0) std::cerr << "Invalid arguments" << std::endl;
    }
  } else if (splitter == 'l') {
    vpsort::parallel_samplesort (work, work+mysize, std::less<double>(), dist,
				stl_sort, 
				nproc, nproc,
				MPI_COMM_WORLD);
    vpsort::print_perf_data_samplesort (dist, stl_sort, MPI_COMM_WORLD);
  } else {
    if (rank == 0) std::cerr << "Invalid arguments" << std::endl;
  }

  check_result (work, mysize);
  if (rank == 0) std::cout << std::endl;

  // Cleanup
  delete [] work;
  MPI_Finalize();

  return 0;
}
