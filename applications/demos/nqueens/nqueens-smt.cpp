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
#include <SmartPointer.hpp>


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
  Board(size_t size)
  {
    this->size = size;
    columns = new int[size];
  }

  ~Board() { delete [] columns; }


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

  size_t Size() const { return size; }   // getter method

  // TODO: bounds checking
  void Set(size_t pos, int val)
  {
    columns[pos] = val;
  }

  int At(size_t pos) const
  {
    assert(pos < size);
    return columns[pos];
  }

  // Inserts a queen in the last column at 'rowIndex'
  void insertQueen(int rowIndex)
  {
    columns[size-1] = rowIndex;
  }

private:
  int *columns; /* has the row position for the queen on each column */
  size_t size;  /* board size */
};

 
template<>
Board *clone<Board>(GlobalAddress<Board> source)
{
  /* read the remote board size */
  int remsize = delegate::call(source, [](Board &b) { return b.Size(); });

  /* create a new board with an extra column to place the next queen */
  Board *local = new Board(remsize+1);

  /* copy the contents of the remote board to the local one */
  for (auto k=0; k<remsize; k++)
    local->Set(k, delegate::call(source, [k](Board &b) {
          return b.At(k);
        }));
 
  return local;
}


/*
 * This is basically a brute force solution using recursion.
 *
 * This procedure creates a new copy of the board, adds a new column
 * (with a queen at the row given by 'rowIndex') and check whether
 * it is safe to add a new queen in any of the rows of the next column.
 * If it is safe, it then recursively spawn new tasks to do the job.
 */
void nqSearch(SmtPtr<Board> remoteBoard, int rowIndex)
{
  /* create a new copy of remoteBoard, adding a new column */
  SmtPtr<Board> newBoard = remoteBoard.clone();

  /* place the queen at row 'rowIndex' in the new column */
  newBoard->insertQueen(rowIndex);
      
  /* are we done yet? */
  if (newBoard->Size() == nqBoardSize) 
    nqSolutions++; // yes, solution found
  else
  { /* not done yet */
  
    /* check whether it is safe to have a queen on row 'i' */
    for (int i=0; i<nqBoardSize; i++) {

      if (newBoard->isSafe(i)) {
      
        /* safe - spawn a new task to check for the next column */
        spawn<unbound,&gce>([newBoard,i] { nqSearch(newBoard, i); });
      }
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
    
    Metrics::reset_all_cores();
    Metrics::start_tracing();

    double start = walltime();

    /* initial empty board */
    SmtPtr<Board> board(new Board(0));
  
    finish<&gce>([board]{
      for (auto i=0; i<nqBoardSize; i++)
      {
        spawn<unbound, &gce>([board,i] { nqSearch(board,i); }); 
      }
    });

    
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

