#include "Hypercube.hpp"
#include <cmath>
#include <iostream>

//TODO: proper path
#include <glog/logging.h>

/*
 * Hypercube implementation
 */

class Hypercube::impl {
  private:
    // length of each dimension
    std::vector<int64_t> shares;

  public:
    impl(std::vector<int64_t> shares) : shares(shares) {} //copy
    HypercubeSlice& slice( std::vector<int64_t> replicandIds ) {
      return *new HypercubeSlice( shares, replicandIds );
    }

};

Hypercube::Hypercube(const std::vector<int64_t> shares) : pimpl(new impl(shares)) {}
Hypercube::~Hypercube() { }
HypercubeSlice& Hypercube::slice( std::vector<int64_t> replicandIds ) { return pimpl->slice(replicandIds); }



/*
 * HypercubeSlice implementation
 */

class HypercubeSlice::impl {
  private:
    const std::vector<int64_t>& shares;
    std::vector<int64_t> replicandIds;

  public:
    impl(const std::vector<int64_t>& shares, std::vector<int64_t> replicandIds)
      : shares(shares)
      , replicandIds(replicandIds) // copy
  {
    CHECK( replicandIds.size() == shares.size() ) << "# of slice dimensions != # of bounds dimensions";
    for (int i=0; i<replicandIds.size(); i++) {
      CHECK( (replicandIds[i] == HypercubeSlice::ALL) || (replicandIds[i] < shares[i]) )
        << "coordinate out of bounds";
    }
  }

    HypercubeSliceIter begin() {
      return HypercubeSliceIter(replicandIds,shares,false);
    }

    HypercubeSliceIter end() {
      return HypercubeSliceIter(replicandIds,shares,true);
    }
    
    //TODO: potential target for memoization
    bool containsSerial(const int64_t x) const {
      // calculate the hypercube coordinate of x
      // O(number of dimensions)
      // checking if it is in the slice along the way
      int64_t val = x;
      for (int i=shares.size(); i>= 0; i--) {
        int64_t coordinate_i = val / shares[i];
        val %= shares[i]; 
      
        // check if the coordinate is contained  
        if ( (replicandIds[i]!=HypercubeSlice::ALL) 
          && (coordinate_i!=replicandIds[i]) ) {
          return false;
        }
      }


      return true;
    }

};

HypercubeSlice::HypercubeSlice(const std::vector<int64_t>& shares, std::vector<int64_t> replicandIds) : pimpl(new impl(shares, replicandIds)) {}
HypercubeSlice::HypercubeSlice(const HypercubeSlice& toCopy) : pimpl(new impl(*(toCopy.pimpl))) {}
HypercubeSlice::~HypercubeSlice() { }
HypercubeSliceIter HypercubeSlice::begin() { return pimpl->begin(); }
HypercubeSliceIter HypercubeSlice::end() { return pimpl->end(); }
bool HypercubeSlice::containsSerial(const int64_t x) { return pimpl->containsSerial(x); }
  
int64_t HypercubeSlice::int_cbrt(int64_t x) {
  int64_t root = round(floor(cbrt(x)));
  int64_t rootCube = root*root*root;
  //if (x != (rootCube) ) { LOG(WARNING) << x << " not a perfect cube; processors 0-"<<(rootCube-1)<<") will count"; }
  return root;
}
const int64_t HypercubeSlice::ALL = -1;


/*
 * Iter implementation
 */
class HypercubeSliceIter::impl {
  private:
    std::vector<int64_t> current_positions;
    std::vector<int64_t> wildcards;
    const std::vector<int64_t>& shares;
    bool done;
        
    //TODO good target for memoizing
    /* Turn the current n-dimensional vector into a sequential id */
    static int64_t serialId (std::vector<int64_t> pos, std::vector<int64_t> sizes) {
      int64_t sum = pos[0];
      int64_t sizeprod = 1;
      DVLOG(5) << "sum start where " << pos.size() << " and " << sizes.size() << std::endl;
      for (int i=1; i<pos.size(); i++) {
        sizeprod *= sizes[i-1];
        sum += pos[i] * sizeprod;
        DVLOG(5) << "sum += " << pos[i] << "*" << sizeprod << std::endl;
      }
      return sum;
    }
    
  public:
    impl(std::vector<int64_t> iteration_box, const std::vector<int64_t>& shares, bool end)    // TODO: would be elegant in Chapel distributions
      : current_positions(iteration_box.size()) 
      , wildcards()
      , shares(shares)
      , done(end) {

        for (int i = 0; i<iteration_box.size(); i++) {
          int64_t p = iteration_box[i];
          if (p==HypercubeSlice::ALL) {
            // if whole dimension then start at 0 to prepare for iteration
            current_positions[i] = 0;
            wildcards.push_back(i);
          } else {
            // fixed dimension
            current_positions[i] = p;
          }
        }
      }

    bool operatorNotEqual (const HypercubeSliceIter::impl& other) const {
      if (done!=other.done) // common case for iteration
        return true;
      else {
       // Only care about done==done for now.
//        for (int i=0; i<current_positions.size(); i++) {
//          if (current_positions[i] != other.current_positions[i]) {
//            return true;
//          }
//        }
        return false;
      }
    }

    const void operatorPlusPlus() {
      int i = 0;
      while (i < wildcards.size()) {
        int64_t dim = wildcards[i];
        if (current_positions[dim]+1 == shares[dim]) {
          current_positions[dim] = 0;
          i++;
        } else {
          //std::cout << "dim="<< dim;
          //std::cout << "current=" << current_positions[0] << ","<<current_positions[1] << ","<<current_positions[2] <<std::endl;
          //std::cout << "share=" << shares[0] << ","<<shares[1] << ","<<shares[2] <<std::endl;
          current_positions[dim]++;
          //std::cout << "current=" << current_positions[0] << ","<<current_positions[1] << ","<<current_positions[2] <<std::endl;
          break;
        }
      }

      // if ran out of dimensions then at end
      if (i == wildcards.size()) {
        done = true;
      }
    }

    int64_t operatorDeref() const {
      return serialId(current_positions, shares);
    }
};

HypercubeSliceIter::HypercubeSliceIter(std::vector<int64_t> iteration_box, const std::vector<int64_t>& shares, bool end) : pimpl(new impl(iteration_box, shares, end)) {}
HypercubeSliceIter::HypercubeSliceIter(const HypercubeSliceIter& toCopy) : pimpl(new impl(*(toCopy.pimpl))) {}
HypercubeSliceIter::~HypercubeSliceIter() {}
bool HypercubeSliceIter::operator!=(const HypercubeSliceIter& other) const { return pimpl->operatorNotEqual(*(other.pimpl)); }
const HypercubeSliceIter& HypercubeSliceIter::operator++() { pimpl->operatorPlusPlus(); return *this; }
int64_t HypercubeSliceIter::operator*() const { return pimpl->operatorDeref(); }

