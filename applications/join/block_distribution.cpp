#include "block_distribution.hpp"

  NaturalNumberRange::NaturalNumberRange(uint64_t leftInclusive, uint64_t rightExclusive)
: leftInclusive(leftInclusive)
  , rightExclusive(rightExclusive) { }



  BlockDistribution::BlockDistribution(uint64_t numBlocks, uint64_t numElements) {
    this->blockSizeMin = numElements/numBlocks;
    this->remainder = numElements%numBlocks;
  }
  
  NaturalNumberRange BlockDistribution::getRangeForBlock(uint64_t blockId) {
    // first remainder blocks get +1 elements
    if (blockId < remainder) {
      uint64_t size = blockSizeMin + 1;
      uint64_t left = (blockSizeMin+1)*blockId;
      return NaturalNumberRange(left, left+size);
    } else {
      // after remainder, blocks get +0 elements
      uint64_t size = blockSizeMin;
      uint64_t left = (blockSizeMin+1)*remainder+blockSizeMin*(blockId-remainder);
      return NaturalNumberRange(left, left+size);
    }
  }
  
uint64_t BlockDistribution::getBlockIdForIndex(uint64_t index) {
    if (index/(blockSizeMin+1) < remainder) {
      return index/(blockSizeMin+1);
    } else {
      return (index-remainder) / blockSizeMin;
    }
  }
