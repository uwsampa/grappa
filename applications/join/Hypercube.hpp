#pragma once
#include <vector>
#include <cstdint>
#include <memory>
    
class HypercubeSliceIter {
  public:
    HypercubeSliceIter(std::vector<int64_t> iteration_box, const std::vector<int64_t>& shares, bool end);
    HypercubeSliceIter(const HypercubeSliceIter& toCopy);
    ~HypercubeSliceIter();
    bool operator!= (const HypercubeSliceIter& other) const;
    const HypercubeSliceIter& operator++();
    int64_t operator*() const;
  private:
    class impl;
    std::unique_ptr<impl> pimpl;
};

class HypercubeSlice {
  public:
    HypercubeSlice(const std::vector<int64_t>& shares, std::vector<int64_t> replicandIds);
    HypercubeSlice(const HypercubeSlice& toCopy);
    ~HypercubeSlice();

    HypercubeSliceIter begin();
    HypercubeSliceIter end();
    bool containsSerial(const int64_t x);

    // get the floor(cube root (x)), with warning for non integers
    static int64_t int_cbrt(int64_t x);
 
    // indicates all values in the dimension 
    static const int64_t ALL;
  
  private:
    class impl;
    std::unique_ptr<impl> pimpl;
};



class Hypercube {
  public:
    Hypercube(std::vector<int64_t> shares);
    ~Hypercube();

    HypercubeSlice& slice( std::vector<int64_t> replicandIds );

  private:
    class impl;
    std::unique_ptr<impl> pimpl;
};

