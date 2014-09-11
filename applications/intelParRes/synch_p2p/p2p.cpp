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

DEFINE_uint64(x, 4, "size of array in X dimension");
DEFINE_uint64(y, 4, "size of array in Y dimension");
DEFINE_uint64(iterations, 1, "number of iterations");

GlobalArray< FullEmpty< double >, Distribution::Block, Distribution::Local > ga;

int main( int argc, char * argv[] ) {
  init( &argc, &argv );
  run([]{
      LOG(INFO) << "Grappa pipeline stencil execution on 2D ("
                << FLAGS_x << "x" << FLAGS_y
                << ") grid";
      
      ga.allocate( FLAGS_x, FLAGS_y );

      double avgtime = 0.0;
      double mintime = 366.0*24.0*3600.0;
      double maxtime = 0.0;

      // initialize
      forall( ga, [] (size_t x, size_t y, FullEmpty<double>& d) {
        if( x == 0 ) {
          d.writeXF( y );
        } else if ( y == 0 ) {
          d.writeXF( x );
        } else {
          d.writeXF( 0.0 );
        }
        });
      
      for( int iter = 0; iter < FLAGS_iterations; ++iter ) {
        // clear inner array cells
        forall( ga, [] (size_t x, size_t y, FullEmpty<double>& d) {
            if( x > 0 && y > 0 ) {
              d.reset();
            }
          } );
        
        // execute kernel
        double start = Grappa::walltime();
        forall( ga, [] (size_t x, size_t y, FullEmpty<double>& d) {
            if( x > 0 && y > 0 ) {
              d.writeXF( readFF( &ga[x-1][y  ] ) +
                         readFF( &ga[x  ][y-1] ) -
                         readFF( &ga[x-1][y-1] ) );
            }
          } );
        double end = Grappa::walltime();
        if( iter > 0 || FLAGS_iterations == 1 ) { // skip the first iteration
          double time = end - start;
          avgtime += time;
          mintime = std::min( mintime, time );
          maxtime = std::max( maxtime, time );
        }

        // copy top right corner value to bottom left corner to create dependency
        writeXF( &ga[0][0], -1.0 * readFF( &ga[ FLAGS_x-1 ][ FLAGS_y-1 ] ) );
      }
      
      // verify result
      double expected_corner_val = (double) FLAGS_iterations * ( FLAGS_x + FLAGS_y - 2 );
      double actual_corner_val = readFF( &ga[ FLAGS_x-1 ][ FLAGS_y-1 ] );
      CHECK_DOUBLE_EQ( actual_corner_val, expected_corner_val );


      avgtime /= (double) std::max( FLAGS_iterations-1, 1ULL );
      LOG(INFO) << "Rate (MFlops/s): " << 1.0E-06 * 2 * ((double)(FLAGS_x-1)*(FLAGS_y-1)) / mintime
                << ", Avg time (s): " << avgtime
                << ", Min time (s): " << mintime
                << ", Max time (s): " << maxtime;
      
      ga.deallocate( );
      
      LOG(INFO) << "Done.";
    });
  finalize();
  return 0;
}
