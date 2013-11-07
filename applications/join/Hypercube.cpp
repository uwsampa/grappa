#include "Hypercube.hpp"
#include <cmath>
//TODO #include <glog/logging.hpp>


/*
 * HypercubeGenerator implementation
 */

class HypercubeGenerator::impl {
  private:
    // length of each dimension
    std::vector<int64_t> shares;

  public:
    impl(std::vector<int64_t> shares) : shares(shares) {} //copy
    Hypercube& gen( std::vector<int64_t> replicandIds ) {
      return *new Hypercube( shares, replicandIds );
    }
};

HypercubeGenerator::HypercubeGenerator(const std::vector<int64_t> shares) : pimpl(new impl(shares)) {}
HypercubeGenerator::~HypercubeGenerator() { }
Hypercube& HypercubeGenerator::gen( std::vector<int64_t> replicandIds ) { return pimpl->gen(replicandIds); }



/*
 * Hypercube implementation
 */

class Hypercube::impl {
  private:
    const std::vector<int64_t>& shares;
    std::vector<int64_t> replicandIds;

  public:
    impl(const std::vector<int64_t>& shares, std::vector<int64_t> replicandIds)
      : shares(shares)
      , replicandIds(replicandIds) // copy
  {}

    HypercubeIter begin() {
      return HypercubeIter(replicandIds,shares,false);
    }

    HypercubeIter end() {
      return HypercubeIter(replicandIds,shares,true);
    }
};

Hypercube::Hypercube(const std::vector<int64_t>& shares, std::vector<int64_t> replicandIds) : pimpl(new impl(shares, replicandIds)) {}
Hypercube::~Hypercube() { }
HypercubeIter Hypercube::begin() { return pimpl->begin(); }
HypercubeIter Hypercube::end() { return pimpl->end(); }
  
int64_t Hypercube::int_cbrt(int64_t x) {
  int64_t root = round(floor(cbrt(x)));
  int64_t rootCube = root*root*root;
  //if (x != (rootCube) ) { LOG(WARNING) << x << " not a perfect cube; processors 0-"<<(rootCube-1)<<") will count"; }
  return root;
}

/*
 * Iter implementation
 */
class HypercubeIter::impl {
  private:
    std::vector<int64_t> current_positions;
    std::vector<int64_t> wildcards;
    const std::vector<int64_t>& shares;
    bool done;
        
    //TODO good target for memoizing
    /* Turn the current n-dimensional vector into a sequential id */
    static int64_t serialId (std::vector<int64_t> pos, std::vector<int64_t> sizes) {
      int64_t sum = pos[0];
      for (int i=1; i<pos.size(); i++) {
        sum += pos[i] * sizes[i-1];
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
          if (p==Hypercube::ALL) {
            // if whole dimension then start at 0 to prepare for iteration
            current_positions[i] = 0;
            wildcards.push_back(p);
          } else {
            // fixed dimension
            current_positions[i] = p;
          }
        }
      }

    bool operatorNotEqual (const HypercubeIter::impl& other) const {
      if (done!=other.done) // common case for iteration
        return true;
      else {
        for (int i=0; i<current_positions.size(); i++) {
          if (current_positions[i] != other.current_positions[i]) {
            return true;
          }
        }
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
          current_positions[dim]++;
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

HypercubeIter::HypercubeIter(std::vector<int64_t> iteration_box, const std::vector<int64_t>& shares, bool end) : pimpl(new impl(iteration_box, shares, end)) {}
HypercubeIter::HypercubeIter(const HypercubeIter& toCopy) : pimpl(new impl(*(toCopy.pimpl))) {}
HypercubeIter::~HypercubeIter() {}
bool HypercubeIter::operator!=(const HypercubeIter& other) const { return pimpl->operatorNotEqual(*(other.pimpl)); }
const HypercubeIter& HypercubeIter::operator++() { pimpl->operatorPlusPlus(); return *this; }
int64_t HypercubeIter::operator*() const { return pimpl->operatorDeref(); }

