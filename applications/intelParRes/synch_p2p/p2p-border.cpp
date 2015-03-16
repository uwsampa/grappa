////////////////////////////////////////////////////////////////////////
// Copyright (c) 2013, Intel Corporation
// Copyright (c) 2014, Jacob Nelson
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions 
// are met:
//
//     * Redistributions of source code must retain the above copyright 
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above 
//       copyright notice, this list of conditions and the following 
//       disclaimer in the documentation and/or other materials provided 
//       with the distribution.
//     * Neither the name of Intel Corporation nor the names of its 
//       contributors may be used to endorse or promote products 
//       derived from this software without specific prior written 
//       permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN 
// ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
// POSSIBILITY OF SUCH DAMAGE.
////////////////////////////////////////////////////////////////////////

#include <Grappa.hpp>
#include <FullEmpty.hpp>
#include <array/GlobalArray.hpp>

using namespace Grappa;

DEFINE_uint64(n, 4, "number of rows in array");
DEFINE_uint64(m, 4, "number of columns in array");
DEFINE_uint64(iterations, 1, "number of iterations");
DEFINE_string(pattern, "border", "what pattern of kernel should we run?");

#define ARRAY(i,j) (local[(j)+((i)*dim2_percore)])

//#define ARRAY(i,j) (local[1+j+i*(dim2_percore)])

double * local = NULL;
int dim1_size = 0;
int dim2_size = 0;
int dim2_percore = 0;

GlobalArray< double, int, Distribution::Local, Distribution::Block > ga;
FullEmpty<double> * lefts = NULL;

double iter_time = 0.0;

int main( int argc, char * argv[] ) {
  init( &argc, &argv );

  run([]{
      LOG(INFO) << "Grappa pipeline stencil execution on 2D ("
                << FLAGS_m << "x" << FLAGS_n
                << ") grid";
      
      ga.allocate( FLAGS_n, FLAGS_m );
      on_all_cores( [] {
          lefts = new FullEmpty<double>[FLAGS_n];
        } );

      double avgtime = 0.0;
      double mintime = 366.0*24.0*3600.0;
      double maxtime = 0.0;

      // initialize
      LOG(INFO) << "Initializing....";
      forall( ga, [] (int i, int j, double& d) {
          if( i == 0 ) {
            d = j;
          } else if ( j == 0 ) {
            d = i;
          } else {
            d = 0.0;
          }
          // LOG(INFO) << "Initx: ga(" << i << "," << j << ")=" << d;
        });
        on_all_cores( [] {
            for( int i = 0; i < FLAGS_n; ++i ) {
              if( (Grappa::mycore() == 0) || 
                  (Grappa::mycore() == 1 && ga.dim2_percore == 1 ) ) {
                lefts[i].writeXF(i);
              } else {
                if( i == 0 ) {
                  lefts[i].writeXF( ga.local_chunk[0] - 1 ); // one to the left of our first element
                } else {
                  lefts[i].reset();
                }
              }
            }
          } );
      
      LOG(INFO) << "Running " << FLAGS_iterations << " iterations....";
      for( int iter = 0; iter < FLAGS_iterations; ++iter ) {
        on_all_cores( [] {
            for( int i = 1; i < FLAGS_n; ++i ) {
              if( ! (Grappa::mycore() == 0) ) {
                lefts[i].reset();
              }
            }
          } );
        
        // execute kernel
        VLOG(2) << "Starting iteration " << iter;
        double start = Grappa::walltime();

        if( FLAGS_pattern == "border" ) {

          finish( [] {
              on_all_cores( [] {
                  local = ga.local_chunk;
                  dim1_size = ga.dim1_size;
                  dim2_size = ga.dim2_size;
                  dim2_percore = ga.dim2_percore;
                  int first_j = Grappa::mycore() * dim2_percore;

                  iter_time = Grappa::walltime();
                  for( int i = 1; i < dim1_size; ++i ) {

                    // prepare to iterate over this segment      
                    double left = readFF( &lefts[i] );
                    double diag = readFF( &lefts[i-1] );
                    double up = 0.0;
                    double current = i;

                    for( int j = 0; j < dim2_percore; ++j ) {
                      int actual_j = j + first_j;
                      if( actual_j > 0 ) {
                        // compute this cell's value
                        up = local[ (i-1)*dim2_percore + j ];
                        current = up + left - diag;

                        // update for next iteration
                        diag = up;
                        left = current;

                        // write value
                        local[ (i)*dim2_percore + j ] = current;
                      }
                    }

                    // if we're at the end of a segment, write to corresponding full bit
                    if( Grappa::mycore()+1 < Grappa::cores() ) {
                      delegate::call<async>( Grappa::mycore()+1,
                                             [=] () {
                                               writeXF( &lefts[i], current );
                                             } );
                    }

                  }
                  iter_time = Grappa::walltime() - iter_time;
                  iter_time = reduce<double,collective_min<double>>( &iter_time );

                } );
            } );

          // forall( ga, [] (int i, int j, double& d) {
          //     LOG(INFO) << "Done: ga(" << i << "," << j << ")=" << ARRAY(i,j);
          //   } );
                  
        } else {
          LOG(FATAL) << "unknown kernel pattern " << FLAGS_pattern << "!";
        }
        double end = Grappa::walltime();

        if( iter > 0 || FLAGS_iterations == 1 ) { // skip the first iteration
          //double time = end - start;
          double time = iter_time;
          avgtime += time;
          mintime = std::min( mintime, time );
          maxtime = std::max( maxtime, time );
        }
        
        on_all_cores( [] {
            VLOG(2) << "done with this iteration";
          } );

        // copy top right corner value to bottom left corner to create dependency
        VLOG(2) << "Adding end-to-end dependence for iteration " << iter;
        double val = delegate::read( &ga[ FLAGS_n-1 ][ FLAGS_m-1 ] );
        delegate::write( &ga[0][0], -1.0 * val );
        delegate::call( 0, [val] { lefts[0].writeXF( -1.0 * val ); } );
        if( ga.dim2_percore == 1 ) delegate::call( 1, [val] { lefts[0].writeXF( -1.0 * val ); } );
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
      double actual_corner_val = delegate::read( &ga[ FLAGS_n-1 ][ FLAGS_m-1 ] );
      CHECK_DOUBLE_EQ( actual_corner_val, expected_corner_val );

      on_all_cores( [] {
          if( lefts ) delete [] lefts;
        } );
      ga.deallocate( );
      
      LOG(INFO) << "Done.";
    });
  finalize();
  return 0;
}
