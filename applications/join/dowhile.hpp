#pragma once

class WhileLoopManager {

  private:
    uint64_t _iteration;
    double _start;
    bool _in_iteration;

  public:
    WhileLoopManager();

    void iteration_start();

    void iteration_end();
};
      
