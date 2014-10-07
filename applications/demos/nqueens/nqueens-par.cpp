
#include <Grappa.hpp>

using namespace Grappa;

using namespace std;

DEFINE_int64( n, 8, "Board size" );

// define custom statistics which are logged by the runtime
// (here we're not using these features, just printing them ourselves)
GRAPPA_DEFINE_METRIC( SimpleMetric<double>, gups_runtime, 0.0 );


/*
 * Urghh.. global variables
 */
int64_t nqSolutions = 0;
int nqBoardSize = 1;
GlobalCompletionEvent gce;

/*
 * These should all be 'private' functions
 */

bool nqIsSafe(int c, vector<int> &cols)
{
  int n = cols.size();

  for (auto i=0; i<n; i++) {
    if (c == cols[i] || abs(n-i) == abs(c-cols[i]))
      return false;
  }

  return true;
}



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

      //int vs = delegate::call(cols, [](vector<int> &v) { return v.size(); });

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
  {0, 1, 0, 0, 2, 10, 4, 40, 92, 352, 724, 2680, 14200, 73712, 365596, 2279184, 14772512};

  run([=]{

    double start = walltime();


    if (nqBoardSize > 16) {
    /* TODO: size not valid for now: print something and exit */
    }


    on_all_cores([]{
      nqBoardSize = FLAGS_n; 
      });

    vector<int> *cols = new vector<int>(0);
    GlobalAddress< vector<int> > g_cols = make_global(cols);

    finish<&gce>([g_cols]{
      nqSearch(g_cols);
    });


    int64_t total = reduce<int64_t,collective_add<int64_t>>(&nqSolutions);

    gups_runtime = walltime() - start;

    LOG(INFO) << "NQueens(" << FLAGS_n << ") = " << total << " (should be " << expectedSolutions[nqBoardSize] << ")";
    LOG(INFO) << "Elapsed time: " << gups_runtime.value() << " seconds";
    
    
  });
  finalize();
}

