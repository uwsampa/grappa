#include <Grappa.hpp>
#include <limits>
#include <iomanip>
#include "dowhile.hpp"

WhileLoopManager::WhileLoopManager() 
  : _iteration(0)
    , _start(0.0)
    , _in_iteration(false) { }

void WhileLoopManager::iteration_start() {
  CHECK(!_in_iteration);
  _in_iteration = true;
  _start = Grappa::walltime();
}

void WhileLoopManager::iteration_end() {
  CHECK(_in_iteration);
  VLOG(2) << "Iteration " << _iteration << ". Elapsed " << std::setprecision(15) << (Grappa::walltime() - _start);
  _iteration += 1;
  _in_iteration = false;
}
      
