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

#include <Grappa.hpp>
#include <FullEmpty.hpp>
#include <array/GlobalArray.hpp>

using namespace Grappa;

//#define P2P_DELEGATE
//#define P2P_ARRAY

DEFINE_uint64(m, 4, "number of rows in array");
DEFINE_uint64(n, 4, "number of columns in array");
DEFINE_uint64(iterations, 1, "number of iterations");

struct Cell {
  CountingSemaphore semaphore;
  double data;
};
  
GlobalArray< Cell, int, Distribution::Local, Distribution::Block > ga;

int main( int argc, char * argv[] ) {
  init( &argc, &argv );
  run([]{
      LOG(INFO) << "Grappa pipeline stencil execution on 2D ("
                << FLAGS_m << "x" << FLAGS_n
                << ") grid";
      
      ga.allocate( FLAGS_m, FLAGS_n );

      double avgtime = 0.0;
      double mintime = 366.0*24.0*3600.0;
      double maxtime = 0.0;

      // initialize
      LOG(INFO) << "Initializing....";
      forall( ga, [] (int i, int j, Cell& cell) {
          cell.data = 0.0;
          cell.semaphore.reset(1);
        });
      
      LOG(INFO) << "Running " << FLAGS_iterations << " iterations....";
      for( int iter = 0; iter < FLAGS_iterations; ++iter ) {
        // clear inner array cells
        forall( ga, [] (int i, int j, Cell& cell) {
            if( (i == 0) && (j == 0) ) {
              cell.semaphore.reset(1); // don't wait on this cell!
            } else if( i == 0 ) { // no up dependence, no diag dependence
              cell.semaphore.reset(0); // wait for one message before allowing progress
              cell.data = 1;
            } else if ( j == 0 ) { // no left dependence, no diag dependence
              cell.semaphore.reset(0); // wait for one message before allowing progress
              cell.data = 1;
            } else {
              cell.data = 0.0;
              cell.semaphore.reset(-2); // wait for three messages before allowing progress
            }
          } );
        
        // execute kernel
        VLOG(2) << "Starting iteration " << iter;
        double start = Grappa::walltime();
        const int m = FLAGS_m;
        const int n = FLAGS_n;

        forall( ga, [m,n] (int i, int j, Cell& cell) {
            // LOG(INFO) << "Visiting (" << i << "," << j
            //           << ") at " << &cell
            //           << " with data="<< cell.data
            //           << " and semaphore=" << cell.semaphore.get_value() ;
            cell.semaphore.decrement();
            double data = cell.data;
            auto base = make_linear(&cell);

            // down dependence
            if( i + 1 < m ) {
#ifdef P2P_DELEGATE
#ifdef P2P_ARRAY
              delegate::call<async>( &ga[i+1][j], [data] (Cell& c) {
#else
              delegate::call<async>( base + n, [data] (Cell& c) {
#endif
#else
                  Cell & c = *(&cell + n);
#endif
                  c.data += data;
                  c.semaphore.increment();
#ifdef P2P_DELEGATE
                } );
#endif
            }

            // right dependence
            if( j + 1 < n ) {
#ifdef P2P_DELEGATE
#ifdef P2P_ARRAY
              delegate::call<async>( &ga[i][j+1], [data] (Cell& c) {
#else
              delegate::call<async>( base + 1, [data] (Cell& c) {
#endif
#else
                  Cell & c = *(&cell + 1);
#endif
                  c.data += data;
                  c.semaphore.increment();
#ifdef P2P_DELEGATE
                } );
#endif
            }

            // diag dependence
            if( i + 1 < m && j + 1 < n ) {
#ifdef P2P_DELEGATE
#ifdef P2P_ARRAY
              delegate::call<async>( &ga[i+1][j+1], [data] (Cell& c) {
#else
              delegate::call<async>( base + n + 1, [data] (Cell& c) {
#endif
#else
                  Cell & c = *(&cell + n + 1);
#endif
                  c.data -= data;
                  c.semaphore.increment();
#ifdef P2P_DELEGATE
                } );
#endif
            }

            cell.semaphore.increment();
          } );
        
        double end = Grappa::walltime();

        if( iter > 0 || FLAGS_iterations == 1 ) { // skip the first iteration
          double time = end - start;
          avgtime += time;
          mintime = std::min( mintime, time );
          maxtime = std::max( maxtime, time );
        }
        
        on_all_cores( [] {
            VLOG(2) << "done with this iteration";
          } );

        // copy top right corner value to bottom left corner to create dependency
        VLOG(2) << "Adding end-to-end dependence for iteration " << iter;
        int last_m = FLAGS_m-1;
        int last_n = FLAGS_n-1;
        auto lastval = delegate::read( &ga[ FLAGS_m-1 ][ FLAGS_n-1 ] );
        delegate::write( &ga[0][0], lastval );
        VLOG(2) << "Done with iteration " << iter;
      }
      
      avgtime /= (double) std::max( FLAGS_iterations-1, static_cast<google::uint64>(1) );
      LOG(INFO) << "Rate (MFlops/s): " << 1.0E-06 * 2 * ((double)(FLAGS_m-1)*(FLAGS_n-1)) / mintime
                << ", Avg time (s): " << avgtime
                << ", Min time (s): " << mintime
                << ", Max time (s): " << maxtime;
      
      // verify result
      VLOG(2) << "Verifying result";
      double expected_corner_val = (double) FLAGS_iterations * ( FLAGS_m + FLAGS_n - 2 );
      auto actual_corner_val = delegate::read( &ga[ FLAGS_m-1 ][ FLAGS_n-1 ] );
      CHECK_DOUBLE_EQ( actual_corner_val.data, expected_corner_val );

      ga.deallocate( );
      
      LOG(INFO) << "Done.";
    });
  finalize();
  return 0;
}
