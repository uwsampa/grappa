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
private:
  void init(size_t size)
  {
    this->size = size;
    columns = new int[size];
    ref_count = 1;
  }
  
public:
  Board(size_t size) { init(size); }

  /* Creates a new board, copies the content of 'source' and
   * insert 'newItem' into the last column.
   */
  Board(GlobalAddress<Board> source, int newItem)
  {
    /* read the remote board size */
    int remsize = delegate::call(source, [](Board &b) { return b.size; });

   /* Add an extra column. */
    init(remsize+1);

    /* copy the contents of the remote board to the local one */
    for (auto k=0; k<remsize; k++)
      columns[k] = delegate::call(source, [k](Board &b) {
            return b.columns[k];
          });
    
    /* Insert the new item */
    columns[size-1] = newItem;
  }

  /* Called whenever the object must be erased (i.e., memory freed) */
  void Release() { if (!--ref_count) delete[] columns; }

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

  /* Called explitictly whenever the object is copied. */
  void Shared() { ref_count++; }

  size_t Size() const { return size; }   // getter method

private:
  int *columns; /* has the row position for the queen on each column */
  size_t size;  /* board size */
  int ref_count; /* reference counter to keep track of memory deallocation */
};



/*
 * This is basically a brute force solution using recursion.
 *
 * Given a board ('remoteBoard') we check if placing a queen in any of the
 * rows of the column given by 'columnIndex' is safe. In such cases a new task
 * is spawned recursively for the next column.
 */
void nqSearch(GlobalAddress<Board> remoteBoard, int columnIndex)
{
  /* create a new copy of remoteBoard, adding a new column */
  Board *newBoard = new Board(remoteBoard, columnIndex);
      
  /* are we done yet? */
  if (newBoard->Size() == nqBoardSize) 
    nqSolutions++; // yes, solution found
  else
  { /* not done yet */
  
    GlobalAddress<Board> g_newBoard = make_global(newBoard);
    
    /* check whether it is safe to have a queen on row 'i' */
    for (int i=0; i<nqBoardSize; i++) {

      if (newBoard->isSafe(i)) {
      
        /* safe - spawn a new task to check for the next column */

        newBoard->Shared();  // board is being shared

        /* spawn a recursive search */
        spawn<unbound,&gce>([g_newBoard,i] { nqSearch(g_newBoard, i); });
      }
    }
  }
  
  /* don't need the remote board anymore: try to delete it */
  delegate::call(remoteBoard, [](Board &b) { b.Release(); });

  newBoard->Release();
}


int main(int argc, char * argv[]) {
  init( &argc, &argv );

  const int expectedSolutions[] =
  {0, 1, 0, 0, 2, 10, 4, 40, 92, 352, 724, 2680, 14200, 73712, 365596, \
    2279184, 14772512};
      
  nqBoardSize = FLAGS_n; 
  nqSolutions = 0;


  run([=]{
    
    Metrics::reset_all_cores();
    Metrics::start_tracing();

    double start = walltime();

    /* initial empty board */
    Board *board = new Board(0);
    GlobalAddress<Board> g_board = make_global(board);

    finish<&gce>([g_board,board]{
      for (auto i=0; i<nqBoardSize; i++)
      {
        board->Shared();
        spawn<unbound, &gce>([g_board,i] { nqSearch(g_board,i); }); 
      }
    });

    board->Release();

    
    int64_t total = reduce<int64_t,collective_add<int64_t>>(&nqSolutions);

    nqueens_runtime = walltime() - start;
    
    Metrics::stop_tracing();

    if (nqBoardSize <= 16) {
      LOG(INFO) << "NQueens (" << nqBoardSize << ") = " << total << \
        " (solution is " << (total == expectedSolutions[nqBoardSize] ? "correct)" : "wrong)");
    }
    else
      LOG(INFO) << "NQueens (" << nqBoardSize << ") = " << total;

    LOG(INFO) << "Elapsed time: " << nqueens_runtime.value() << " seconds";
    
    Metrics::merge_and_dump_to_file();
    
  });
  finalize();
}

