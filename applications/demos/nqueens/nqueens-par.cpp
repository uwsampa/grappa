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
 * The main board class.
 *
 * The board is represented as a column array specifying the row position of the
 * Queen on each column.
 */
class Board {
public:
  Board(size_t size): size(size), columns(new int[size]) {}

  /* creates a new board, copies the content of 'source' and
   * insert 'newItem' into the last column
   */
  Board(Board &source, int newItem): Board(source.size+1)
  {
    memcpy(columns, source.columns, source.size);
    columns[size-1] = newItem;
  }

  ~Board() { delete[] columns; }


  /*
   * Checks whether it is safe to put a queen in row 'row'. 
   */
  bool isSafe(int row)
  { 

    /* we check if putting a queen on row 'row' will cause any of the previous
     * queens (in 'columns') to capture it. */
    for (auto i=0; i<size; i++) {
      if (row == columns[i] || abs(size-i) == abs(row-columns[i]))
        return false;  // captured - not safe
    }

    return true;
  }

  int *columns; /* has the row position for the queen on each column */
  size_t size;  /* board size */
};


/*
 * This is basically a brute force solution using recursion.
 *
 * We start with an empty Board (i.e., placing queens on the first column), 
 * check if it is safe to place a queen on a specific row and recursively call
 * nqSearch() to check for the next column.
 */
void nqSearch(GlobalAddress<Board> remoteBoard)
{
  /* read the remote board size */
  int size = delegate::call(remoteBoard, [](Board &b) { return b.size; });
  
  Board localBoard(size);  // create a local board

  /* copy the contents of the remote board to the local one */
  for (auto k=0; k<size; k++)
    localBoard.columns[k] = delegate::call(remoteBoard, [k](Board &b) {
          return b.columns[k];
         });

  /* don't need the remote board anymore: delete it */
  delegate::call(remoteBoard, [](Board &b) { delete &b; });

  /* are we done yet? */
  if (localBoard.size == nqBoardSize) {
    nqSolutions++;
    return;
  }
  
  /* check whether it is safe to have a queen on row 'i' */
  for (int i=0; i<nqBoardSize; i++) {

    if (localBoard.isSafe(i)) {

      /* safe - we create a brand new board (with size+1), copying the contents
       * of the local board, add the queen in row 'i', and spawn a new
       * task to check for the next column */
      Board *newBoard = new Board(localBoard, i);

      GlobalAddress<Board> g_newBoard = make_global(newBoard);

      /* spawn a recursive search */
      spawn<unbound,&gce>([g_newBoard] { nqSearch(g_newBoard); });
    }
  }
      
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

    /* initial empty board */
    Board *board = new Board(0);
    GlobalAddress<Board> g_board = make_global(board);

    finish<&gce>([g_board]{
      nqSearch(g_board);
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

