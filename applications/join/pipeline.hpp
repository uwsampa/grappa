#pragma once
#include <Grappa.hpp>
#include <CompletionEvent.hpp>
#include <functional>

class Pipeline {
  private:
  std::function<void(void)> _f;
  uint64_t _id;
  std::vector<Pipeline*> _wait_for;
  Grappa::CompletionEvent _p_task_ce;
  
  public:
  Pipeline(uint64_t id, std::vector<Pipeline*> wait_for, std::function<void(void)> f);
  void run();
  void wait();
};
    
