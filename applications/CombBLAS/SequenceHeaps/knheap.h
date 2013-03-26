// hierarchical memory priority queue data structure
#ifndef KNHEAP
#define KNHEAP
#include "util.h"

const int KNBufferSize1 = 32; 	// equalize procedure call overheads etc. 
const int KNN = 128; 		// bandwidth (also size of the binary heap)
const int KNKMAX = 128;  	// maximal arity
const int KNLevels = 4; 	// overall capacity >= KNN*KNKMAX^KNLevels
const int LogKNKMAX = 7;  	// ceil(log KNK)
/*
const int KNBufferSize1 = 3; // equalize procedure call overheads etc. 
const int KNN = 10; // bandwidth
const int KNKMAX = 4;  // maximal arity
const int KNLevels = 4; // overall capacity >= KNN*KNKMAX^KNLevels
const int LogKNKMAX = 2;  // ceil(log KNK)
*/
template <class Key, class Value>
struct KNElement {Key key; Value value;};

//////////////////////////////////////////////////////////////////////
// fixed size binary heap
template <class Key, class Value, int capacity>
class BinaryHeap {
  //  static const Key infimum  = 4;
  //static const Key supremum = numeric_limits<Key>.max();
  typedef KNElement<Key, Value> Element;
  Element data[capacity + 2];
  int size;  // index of last used element
public:
  BinaryHeap(Key sup, Key infimum):size(0) { 
    data[0].key = infimum; // sentinel
    data[capacity + 1].key = sup;
    reset();
  }
  Key getSupremum() { return data[capacity + 1].key; }
  void reset();
  int   getSize()     const { return size; }
  Key   getMinKey()   const { return data[1].key; }
  Value getMinValue() const { return data[1].value; }
  void  deleteMin();
  void  deleteMinFancy(Key *key, Value *value) {
    *key   = getMinKey();
    *value = getMinValue();
    deleteMin();
  }
  void  insert(Key k, Value v);
  void  sortTo(Element *to); // sort in increasing order and empty
  //void  sortInPlace(); // in decreasing order
};


// reset size to 0 and fill data array with sentinels
template <class Key, class Value, int capacity>
inline void BinaryHeap<Key, Value, capacity>::
reset() {
  size = 0;
  Key sup = getSupremum();
  for (int i = 1;  i <= capacity;  i++) {
    data[i].key = sup;
  }
  // if this becomes a bottle neck
  // we might want to replace this by log KNN 
  // memcpy-s
}

template <class Key, class Value, int capacity>
inline void BinaryHeap<Key, Value, capacity>::
deleteMin()
{
  Assert2(size > 0);

  // first move up elements on a min-path
  int hole = 1; 
  int succ = 2;
  int sz   = size;
  while (succ < sz) {
    Key key1 = data[succ].key;
    Key key2 = data[succ + 1].key;
    if (key1 > key2) {
      succ++;
      data[hole].key   = key2;
      data[hole].value = data[succ].value;
    } else {
      data[hole].key   = key1;
      data[hole].value = data[succ].value;
    }
    hole = succ;
    succ <<= 1;
  }

  // bubble up rightmost element
  Key bubble = data[sz].key;
  int pred = hole >> 1;
  while (data[pred].key > bubble) { // must terminate since min at root
    data[hole] = data[pred];
    hole = pred;
    pred >>= 1;
  }

  // finally move data to hole
  data[hole].key = bubble;
  data[hole].value = data[sz].value;

  data[size].key = getSupremum(); // mark as deleted
  size = sz - 1;
}


// empty the heap and put the element to "to"
// sorted in increasing order
template <class Key, class Value, int capacity>
inline void BinaryHeap<Key, Value, capacity>::
sortTo(Element *to)
{
  const int           sz = size;
  const Key          sup = getSupremum();
  Element * const beyond = to + sz;
  Element * const root   = data + 1;
  while (to < beyond) {
    // copy minimun
    *to = *root;
    to++;

    // bubble up second smallest as in deleteMin
    int hole = 1;
    int succ = 2;
    while (succ <= sz) {
      Key key1 = data[succ    ].key;
      Key key2 = data[succ + 1].key;
      if (key1 > key2) {
        succ++;
        data[hole].key   = key2;
        data[hole].value = data[succ].value;
      } else {
        data[hole].key = key1;
        data[hole].value = data[succ].value;
      }
      hole = succ;
      succ <<= 1;
    }

    // just mark hole as deleted
    data[hole].key = sup;
  }
  size = 0;
}


template <class Key, class Value, int capacity>
inline void BinaryHeap<Key, Value, capacity>::
insert(Key k, Value v)
{
  Assert2(size < capacity);
  Debug4(cout << "insert(" << k << ", " << v << ")" << endl);

  size++;
  int hole = size; 
  int pred = hole >> 1;
  Key predKey = data[pred].key;
  while (predKey > k) { // must terminate due to sentinel at 0
    data[hole].key   = predKey;
    data[hole].value = data[pred].value;
    hole = pred;
    pred >>= 1;
    predKey = data[pred].key;
  }

  // finally move data to hole
  data[hole].key   = k;
  data[hole].value = v;
}

//////////////////////////////////////////////////////////////////////
// The data structure from Knuth, "Sorting and Searching", Section 5.4.1
template <class Key, class Value>
class KNLooserTree {
  // public: // should not be here but then I would need a scary
  // sequence of template friends which I doubt to work
  // on all compilers
  typedef KNElement<Key, Value> Element;
  struct Entry   {
    Key key;   // Key of Looser element (winner for 0)
    int index; // number of loosing segment
  };

  // stack of empty segments
  int empty[KNKMAX]; // indices of empty segments
  int lastFree;  // where in "empty" is the last valid entry?

  int size; // total number of elements stored
  int logK; // log of current tree size
  int k; // invariant k = 1 << logK

  Element dummy; // target of empty segment pointers

  // upper levels of looser trees
  // entry[0] contains the winner info
  Entry entry[KNKMAX]; 

  // leaf information
  // note that Knuth uses indices k..k-1
  // while we use 0..k-1
  Element *current[KNKMAX]; // pointer to actual element
  Element *segment[KNKMAX]; // start of Segments

  // private member functions
  int initWinner(int root);
  void updateOnInsert(int node, Key newKey, int newIndex, 
                      Key *winnerKey, int *winnerIndex, int *mask);
  void deallocateSegment(int index);
  void doubleK();
  void compactTree();
  void rebuildLooserTree();
  int segmentIsEmpty(int i);
public:
  KNLooserTree();
  void init(Key sup); // before, no consistent state is reached :-(

  void multiMergeUnrolled3(Element *to, int l);
  void multiMergeUnrolled4(Element *to, int l);
  void multiMergeUnrolled5(Element *to, int l);
  void multiMergeUnrolled6(Element *to, int l);
  void multiMergeUnrolled7(Element *to, int l);
  void multiMergeUnrolled8(Element *to, int l);
  void multiMergeUnrolled9(Element *to, int l);
  void multiMergeUnrolled10(Element *to, int l);

  void multiMerge(Element *to, int l); // delete l smallest element to "to"
  void multiMergeK(Element *to, int l); 
  int  spaceIsAvailable() { return k < KNKMAX || lastFree >= 0; } 
     // for new segment
  void insertSegment(Element *to, int sz); // insert segment beginning at to
  int  getSize() { return size; }
  Key getSupremum() { return dummy.key; }
};  


//////////////////////////////////////////////////////////////////////
// 2 level multi-merge tree
template <class Key, class Value>
class KNHeap {
  typedef KNElement<Key, Value> Element;

  KNLooserTree<Key, Value> tree[KNLevels];

  // one delete buffer for each tree (extra space for sentinel)
  Element buffer2[KNLevels][KNN + 1]; // tree->buffer2->buffer1
  Element *minBuffer2[KNLevels];

  // overall delete buffer
  Element buffer1[KNBufferSize1 + 1];
  Element *minBuffer1;

  // insert buffer
  BinaryHeap<Key, Value, KNN> insertHeap;

  // how many levels are active
  int activeLevels;
  
  // total size not counting insertBuffer and buffer1
  int size;

  // private member functions
  void refillBuffer1();
  void refillBuffer11(int sz);
  void refillBuffer12(int sz);
  void refillBuffer13(int sz);
  void refillBuffer14(int sz);
  int refillBuffer2(int k);
  int makeSpaceAvailable(int level);
  void emptyInsertHeap();
  Key getSupremum() const { return buffer2[0][KNN].key; }
  int getSize1( ) const { return ( buffer1 + KNBufferSize1) - minBuffer1; }
  int getSize2(int i) const { return &(buffer2[i][KNN])     - minBuffer2[i]; }
public:
  KNHeap(Key sup, Key infimum);
  int   getSize() const;
  void  getMin(Key *key, Value *value);
  void  deleteMin(Key *key, Value *value);
  void  insert(Key key, Value value);
};


template <class Key, class Value>  
inline int KNHeap<Key, Value>::getSize() const 
{ 
  return 
    size + 
    insertHeap.getSize() + 
    ((buffer1 + KNBufferSize1) - minBuffer1); 
}

template <class Key, class Value>
inline void  KNHeap<Key, Value>::getMin(Key *key, Value *value) {
  Key key1 = minBuffer1->key;
  Key key2 = insertHeap.getMinKey();
  if (key2 >= key1) {
    *key   = key1;
    *value = minBuffer1->value;
  } else {
    *key   = key2;
    *value = insertHeap.getMinValue();
  }
}

template <class Key, class Value>
inline void  KNHeap<Key, Value>::deleteMin(Key *key, Value *value) {
  Key key1 = minBuffer1->key;
  Key key2 = insertHeap.getMinKey();
  if (key2 >= key1) {
    *key   = key1;
    *value = minBuffer1->value;
    Assert2(minBuffer1 < buffer1 + KNBufferSize1); // no delete from empty
    minBuffer1++;
    if (minBuffer1 == buffer1 + KNBufferSize1) {
      refillBuffer1();
    }
  } else {
    *key = key2;
    *value = insertHeap.getMinValue();
    insertHeap.deleteMin();
  }
}

template <class Key, class Value>
inline  void  KNHeap<Key, Value>::insert(Key k, Value v) {
  if (insertHeap.getSize() == KNN) { emptyInsertHeap(); }
  insertHeap.insert(k, v);
}
#endif
