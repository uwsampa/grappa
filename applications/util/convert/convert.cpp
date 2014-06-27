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
