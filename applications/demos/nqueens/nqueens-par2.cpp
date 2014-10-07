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

using namespace Grappa;
using namespace std;


DEFINE_int64( n, 8, "Board size" );

GRAPPA_DEFINE_METRIC( SimpleMetric<double>, nqueens_runtime, 0.0 );


/*
 * Each core maintains the number of solutions it has found + the board size
 */
int64_t nqSolutions;
int nqBoardSize;
GlobalCompletionEvent gce;      // tasks synchronization


/*
 * Checks whether it is safe to put a queen in row 'row' in the current 
 * column. Vector 'cols' has the row position for the queens in the previous
 * columns.
 */
bool nqIsSafe(int row, vector<int> &cols)
{
  int n = cols.size();

  /* we check if putting a queen on row 'row' will cause any of the previous
   * queens (in 'cols') to capture it.
   */
  for (auto i=0; i<n; i++) {
    if (row == cols[i] || abs(n-i) == abs(row-cols[i]))
      return false;  // captured - not safe
  }

  return true;
}


/*
 * This is basically a brute force solution using recursion.
 *
 * Vector 'cols' stores, for each column, the row number of a queen. We start
 * with an empty vector (i.e., placing queens on the first column), check if
 * it is safe to place a queen on a specific row and recursively call nqSearch
 * to check for the next column.
 */
void nqSearch(GlobalAddress< vector<int> > cols)
{

  /* get the size of the vector */
  int vsize = delegate::call(cols, [](vector<int> &v) { return v.size(); });

  /* are we done yet? */
  if (vsize == nqBoardSize) {
    nqSolutions++;
    return;
  }

  
  /* check whether it is safe to consider a queen for the next column */
  for (int i=0; i<nqBoardSize; i++) {

    /* safe? code is executed at the core that owns the vector */
    bool safe = delegate::call(cols, [i](vector<int> &v) {
      bool safe = nqIsSafe(i, v);
      return safe;
      });

    /* if not safe we are done - otherwise create a new vector and do
     * a recursive call to nqSearch */
    if (safe) {

      vector<int> *mv = new vector<int>(0);

      /* copy the original vector to the new one we just created */
      for (auto k=0; k<vsize; k++) {
        mv->push_back(delegate::call(cols, [k](vector<int> &v) { return v[k]; }));
      }
      mv->push_back(i);

      GlobalAddress< vector<int> > g_mv = make_global(mv);

      /* spawn a recursive search */
      spawn<unbound,&gce>([g_mv] { nqSearch(g_mv); });
    }
  }
     
  /* delete original vector */
  delegate::call(cols, [](vector<int> &v) { delete &v; });

}



int main(int argc, char * argv[]) {
  init( &argc, &argv );

  const int expectedSolutions[] =
  {0, 1, 0, 0, 2, 10, 4, 40, 92, 352, 724, 2680, 14200, 73712, 365596, \
    2279184, 14772512};
      
  nqBoardSize = FLAGS_n; 
  nqSolutions = 0;

  run([=]{

    double start = walltime();


    vector<int> *cols = new vector<int>(0);
    GlobalAddress< vector<int> > g_cols = make_global(cols);

    finish<&gce>([g_cols]{
      nqSearch(g_cols);
    });


    int64_t total = reduce<int64_t,collective_add<int64_t>>(&nqSolutions);

    nqueens_runtime = walltime() - start;

    if (nqBoardSize <= 16) {
      LOG(INFO) << "NQueens (" << nqBoardSize << ") = " << total << \
        " (solution is " << (total == expectedSolutions[nqBoardSize] ? "correct)" : "wrong)");
    }
    else
      LOG(INFO) << "NQueens (" << nqBoardSize << ") = " << total;

    LOG(INFO) << "Elapsed time: " << nqueens_runtime.value() << " seconds";
    
    
  });
  finalize();
}

