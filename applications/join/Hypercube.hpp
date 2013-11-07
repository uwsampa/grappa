#pragma once
#include <vector>
#include <cstdint>
#include <memory>
    
class HypercubeIter {
  public:
    HypercubeIter(std::vector<int64_t> iteration_box, const std::vector<int64_t>& shares, bool end);
    // need to override the default copy constructor because
    // pimpl needs to be copied
    HypercubeIter(const HypercubeIter& toCopy);
    ~HypercubeIter();
    bool operator!= (const HypercubeIter& other) const;
    const HypercubeIter& operator++();
    int64_t operator*() const;
  private:
    class impl;
    std::unique_ptr<impl> pimpl;
};

class Hypercube {
  public:
    Hypercube(const std::vector<int64_t>& shares, std::vector<int64_t> replicandIds);
    ~Hypercube();

    HypercubeIter begin();
    HypercubeIter end();

    // get the floor(cube root (x)), with warning for non integers
    static int64_t int_cbrt(int64_t x);
 
    // indicates all values in the dimension 
    static const int64_t ALL = -1;
  
  private:
    class impl;
    std::unique_ptr<impl> pimpl;
};



class HypercubeGenerator {
  public:
    HypercubeGenerator(std::vector<int64_t> shares);
    ~HypercubeGenerator();

    Hypercube& gen( std::vector<int64_t> replicandIds );

  private:
    class impl;
    std::unique_ptr<impl> pimpl;
};

