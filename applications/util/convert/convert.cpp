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

// this program converts between edgelist file formats.
// if no input path is specified, it generates a Kronkecker graph.

#include <Grappa.hpp>
#include <graph/Graph.hpp>

using namespace Grappa;

// file specifications
DEFINE_string(input_path, "", "Path to graph source file");
DEFINE_string(input_format, "bintsv4", "Format of graph source file");
DEFINE_string(output_path, "", "Path to graph destination file");
DEFINE_string(output_format, "bintsv4", "Format of graph destination file");

// settings for generator
DEFINE_uint64( nnz_factor, 16, "Approximate number of non-zeros per matrix row" );
DEFINE_uint64( scale, 16, "logN dimension of square matrix" );


int main(int argc, char* argv[]) {
  Grappa::init(&argc, &argv);
  Grappa::run([]{
    TupleGraph tg;
    double t = walltime();

    if( FLAGS_input_path.empty() ) {
      uint64_t N = (1L<<FLAGS_scale);
      uint64_t desired_nnz = FLAGS_nnz_factor * N;
      long userseed = 0xDECAFBAD; // from (prng.c: default seed)
      tg = TupleGraph::Kronecker(FLAGS_scale, desired_nnz, userseed, userseed);
    } else {
      // load from file
      tg = TupleGraph::Load( FLAGS_input_path, FLAGS_input_format );
    }

    t = walltime() - t;
    LOG(INFO) << "Loaded " << tg.nedge << " edges in " << t << " seconds.";

    if( !FLAGS_output_path.empty() ) {
      t = walltime();
      tg.save( FLAGS_output_path, FLAGS_output_format );
      t = walltime() - t;
      LOG(INFO) << "Saved " << tg.nedge << " edges in " << t << " seconds.";
    }
    
  });
  Grappa::finalize();
}
