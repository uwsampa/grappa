#include <cstdint>

class NaturalNumberRange {
  public:
    const uint64_t leftInclusive;
    const uint64_t rightExclusive;

    NaturalNumberRange(uint64_t leftInclusive, uint64_t rightExclusive);
};

class BlockDistribution {
  private:
   uint64_t blockSizeMin;
   uint64_t remainder;

public:

  /**
   * Create a block distribution of elements.
   * Each block contains contiguous elements.
   *
   * @param numBlocks number of blocks to distribute across
   * @param numElements number of elements
   */
  BlockDistribution(uint64_t numBlocks, uint64_t numElements);

  NaturalNumberRange getRangeForBlock(uint64_t blockId);

  uint64_t getBlockIdForIndex(uint64_t index);
};
