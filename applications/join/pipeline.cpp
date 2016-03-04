#include <iostream>
#include <iomanip>
#include "pipeline.hpp"

using namespace Grappa;

Pipeline::Pipeline(uint64_t id, std::vector<Pipeline*> wait_for, std::function<void(void)> f) 
    : _f(f)
    , _id(id)
    , _wait_for(wait_for)
    , _p_task_ce() { }

void Pipeline::run() {
  spawn(&_p_task_ce, [=] {
    for (auto w : _wait_for) {
      w->wait();
    }
    
    auto start = walltime();
    VLOG(1) << "timestamp " << _id << " start " << std::setprecision(15) << start;

    _f(); 

    auto end = walltime();
    auto runtime = end - start;

    VLOG(1) << "pipeline " << _id << ": " << runtime << " s";
    VLOG(1) << "timestamp " << _id << " end " << std::setprecision(15) << end;
  });
}

void Pipeline::wait() {
  _p_task_ce.wait();
}
