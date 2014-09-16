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

DEFINE_uint64(m, 4, "number of rows in array");
DEFINE_uint64(n, 4, "number of columns in array");
DEFINE_uint64(iterations, 1, "number of iterations");
DEFINE_string(pattern, "blocking_reads", "what pattern of kernel should we run?");

GlobalArray< FullEmpty< double >, Distribution::Local, Distribution::Block > ga;

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
      forall( ga, [] (size_t i, size_t j, FullEmpty<double>& d) {
          if( i == 0 ) {
            d.writeXF( j );
          } else if ( j == 0 ) {
            d.writeXF( i );
          }
        });
      
      LOG(INFO) << "Running " << FLAGS_iterations << " iterations....";
      for( int iter = 0; iter < FLAGS_iterations; ++iter ) {
        // clear inner array cells
        forall( ga, [] (size_t i, size_t j, FullEmpty<double>& d) {
            if( i > 0 && j > 0 ) {
              d.reset();
            }
          } );
        
        // execute kernel
        LOG(INFO) << "Starting iteration " << iter;
        double start = Grappa::walltime();

        if( FLAGS_pattern == "blocking_reads" ) {
          forall( ga, [] (size_t i, size_t j, FullEmpty<double>& d) {
              if( i > 0 && j > 0 ) {
                CHECK_EQ( make_global(&d), &ga[i][j] ) << "At (" << i << "," << j << ")";
                LOG(INFO) << "Accessing cell (" << i << "," << j << ")";
                d.writeXF( readFF( &ga[i-1][j  ] ) +
                           readFF( &ga[i  ][j-1] ) -
                           readFF( &ga[i-1][j-1] ) );
                LOG(INFO) << "Cell (" << i << "," << j << ") @ " << make_global(&d) << " == " << d.readFF();
              }
            } );

        // } else if ( FLAGS_pattern == "spawns" ) {
        //   forall( ga, [] (size_t i, size_t j, FullEmpty<double>& d) {
        //       // once we're here, this cell (i,j) must have seen
        //       // updates from all three neighbors.
        //       double data = d.readFF();

        //       if( i < FLAGS_m-1 ) {
        //         // block until cell below us has gotten updates from its
        //         // left and diag neighbors
        //         double i_plus_1_data = ga[i+1][j]->readFF();
                
        //         // add in our data (down neighbor). 
        //         i_plus_1_data += data;

        //         // fill in cell (i+1,j); it's now complete.
        //         ga[i+1][j]->writeFF( i_plus_1_data );

        //         // now generate (combined) left and diag dependences for cell (i+1,j+1).
        //         if( j < FLAGS_n-1 ) {
        //           call<async>( &ga[i+1][j+1], [=] (FullEmpty<double>& dd) {
        //               dd.writeXF( i_plus_1_data - data );  // now cell waits for down dependence.
        //           } );
        //         }
        //       }
        //     } );

        } else {
          LOG(FATAL) << "unknown kernel pattern " << FLAGS_pattern << "!";
        }
        double end = Grappa::walltime();

        if( iter > 0 || FLAGS_iterations == 1 ) { // skip the first iteration
          double time = end - start;
          avgtime += time;
          mintime = std::min( mintime, time );
          maxtime = std::max( maxtime, time );
        }
        
        on_all_cores( [] {
            LOG(INFO) << "done with this iteration";
          } );

        // copy top right corner value to bottom left corner to create dependency
        LOG(INFO) << "Adding end-to-end dependence for iteration " << iter;
        //writeXF( &ga[0][0], -1.0 * readFF( &ga[ FLAGS_m-1 ][ FLAGS_n-1 ] ) );
        size_t last_m = FLAGS_m-1;
        size_t last_n = FLAGS_n-1;
        auto a = &ga[ FLAGS_m-1 ][ FLAGS_n-1 ];
          //double val = readFF( &ga[ FLAGS_m-1 ][ FLAGS_n-1 ] );
        //double val = readFF( &ga[ 1 ][ 9 ] );
          double val = readFF( a );
        writeXF( &ga[0][0], -1.0 * val );
        LOG(INFO) << "Done with iteration " << iter;
      }
      
      // verify result
      LOG(INFO) << "Verify";
      double expected_corner_val = (double) FLAGS_iterations * ( FLAGS_m + FLAGS_n - 2 );
      //double actual_corner_val = readFF( &ga[ FLAGS_m-1 ][ FLAGS_n-1 ] );
      //double actual_corner_val = readFF( &ga[ 1 ][ 9 ] );
      auto aa = &ga[ 1 ][ 9 ];
      double actual_corner_val = readFF( aa );
      CHECK_DOUBLE_EQ( actual_corner_val, expected_corner_val );

      avgtime /= (double) std::max( FLAGS_iterations, static_cast<google::uint64>(1) );
      LOG(INFO) << "Rate (MFlops/s): " << 1.0E-06 * 2 * ((double)(FLAGS_m-1)*(FLAGS_n-1)) / mintime
                << ", Avg time (s): " << avgtime
                << ", Min time (s): " << mintime
                << ", Max time (s): " << maxtime;
      
      ga.deallocate( );
      
      LOG(INFO) << "Done.";
    });
  finalize();
  return 0;
}
