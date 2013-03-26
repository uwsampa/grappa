// 4ary heap data structure
#ifndef HEAP4
#define HEAP4
#include "util.h"

const unsigned long LineSize = 64; // cache line size (or multiple)

template <class Key, class Value>
struct KNElement {Key key; Value value;};

// align an address
// require: sz is a power of two
inline char *knAlign(void *p, unsigned long sz)
{
  return (char*)(((unsigned long)p + (sz - 1)) & ~(sz - 1));
}//////////////////////////////////////////////////////////////////////
// fixed size 4-ary heap
template <class Key, class Value>
class Heap4 {
  //  static const Key infimum  = 4;
  //static const Key supremum = numeric_limits<Key>.max();
  typedef KNElement<Key, Value> Element;
  int capacity;
  Element * rawData;
  Element * const data; // aligned version of rawData
  int size;  // index of last used element
  int finalLayerSize; // size of first layer with free space
  int finalLayerDist; // distance to end of layer
public:
  Heap4(Key sup, Key infimum, int cap) :
    capacity(cap), 
    rawData(new Element[capacity + 4 + (LineSize-1)/sizeof(Element) + 1]),
    data((Element*)knAlign(rawData, LineSize))
  { 
    data[0].key = infimum; // sentinel
    data[capacity + 1].key = sup;
    data[capacity + 2].key = sup;
    data[capacity + 3].key = sup;
    reset();
  }

  ~Heap4() { delete [] rawData; }
  Key getSupremum() { return data[capacity + 1].key; }
  void reset();
  int   getSize()     const { return size; }
  Key   getMinKey()   const { return data[1].key; }
  Value getMinValue() const { return data[1].value; }
  void  deleteMinBasic();
  void  deleteMin(Key *key, Value *value);
  void  insert(Key k, Value v);
  void  print();
  //void  sortTo(Element *to); // sort in increasing order and empty
  //void  sortInPlace(); // in decreasing order
};


// reset size to 0 and fill data array with sentinels
template <class Key, class Value>
inline void Heap4<Key, Value>::
reset() {
  size = 0;
  finalLayerSize = 1;
  finalLayerDist = 0; 
  Key sup = getSupremum();
  for (int i = 1;  i <= capacity;  i++) {
    data[i].key = sup;
  }
}

template <class Key, class Value>
inline void Heap4<Key, Value>::
deleteMin(Key *key, Value *value)
{
  *key   = getMinKey();
  *value = getMinValue();
  deleteMinBasic();
}

template <class Key, class Value>
inline void Heap4<Key, Value>::
deleteMinBasic()
{
  Assert2(size > 0);
  Key minKey, otherKey;
  int delta;

  // first move up elements on a min-path
  int hole = 1; 
  int succ = 2;
  int layerSize = 4; // size of succ's layer
  int layerPos  = 0; // pos of succ within its layer
  int sz   = size;
  size = sz - 1;
  finalLayerDist++;
  if (finalLayerDist == finalLayerSize) { // layer empty now
    finalLayerSize >>= 2;
    finalLayerDist = 0;
  }
  while (succ < sz) {
    minKey = data[succ].key;
    delta = 0;

    // I could save a few assignments using 
    // a complete case distincition but
    // this costs in terms of instruction cache load
    otherKey = data[succ + 1].key;
    if (otherKey < minKey) { minKey = otherKey; delta = 1; }
    otherKey = data[succ + 2].key;
    if (otherKey < minKey) { minKey = otherKey; delta = 2; }
    otherKey = data[succ + 3].key;
    if (otherKey < minKey) { minKey = otherKey; delta = 3; }
    succ += delta;
    layerPos += delta;

    // move min successor up
    data[hole].key = minKey;
    data[hole].value = data[succ].value;

    // step to next layer
    hole = succ;
    succ = succ - layerPos + layerSize; // beginning of next layer
    layerPos <<= 2;
    succ += layerPos; // now correct value
    layerSize <<= 2;
  }

  // bubble up rightmost element
  Key bubble = data[sz].key;
  layerSize >>= 2; // now size of hole's layer
  layerPos  >>= 2; // now pos of hole within its layer
  // Assert2(finalLayerSize == layerSize);
  int layerDist = layerSize - layerPos - 1; // hole's dist to end of lay.
  int pred = hole + layerDist - layerSize; // end of pred's layer for now
  layerSize >>= 2; // now size of pred's layer
  layerDist >>= 2; // now pred's pos in layer
  pred = pred - layerDist; // finally preds index 
  while (data[pred].key > bubble) { // must terminate since inf at root
    data[hole] = data[pred];
    hole = pred;
    pred = hole + layerDist - layerSize; // end of hole's layer for now
    layerSize >>= 2; // now size of pred's layer
    layerDist >>= 2; 
    pred = pred - layerDist; // finally preds index 
  }

  // finally move data to hole
  data[hole].key = bubble;
  data[hole].value = data[sz].value;

  data[sz].key = getSupremum(); // mark as deleted
}


template <class Key, class Value>
void Heap4<Key, Value>::
insert(Key k, Value v)
{
  Assert2(size < capacity);
  Debug4(cout << "insert(" << k << ", " << v << ")" << endl);

  int layerSize = finalLayerSize;
  int layerDist  = finalLayerDist;
  finalLayerDist--;
  if (finalLayerDist == -1) { // layer full
    // start next layer
    finalLayerSize <<= 2;
    finalLayerDist = finalLayerSize - 1;
  }
  size++;
  int hole = size; 
  int pred = hole + layerDist - layerSize; // end of preds's layer for now
  layerSize >>= 2; // now size of pred's layer
  layerDist >>= 2; 
  pred = pred - layerDist; // finally preds index 
  Key predKey = data[pred].key;
  while (predKey > k) { // must terminate due to sentinel at 0
    data[hole].key   = predKey;
    data[hole].value = data[pred].value;
    hole = pred;
    pred = hole + layerDist - layerSize; // end of preds's layer for now
    layerSize >>= 2; // now size of pred's layer
    layerDist >>= 2; 
    pred = pred - layerDist; // finally preds index 
    predKey = data[pred].key;
  }

  // finally move data to hole
  data[hole].key   = k;
  data[hole].value = v;
}

template <class Key, class Value>
inline void Heap4<Key, Value>::
print()
{
  int pos = 1;
  for (int layerSize = 1;  pos < size;  layerSize <<= 2) {
    for (int i = 0;  i < layerSize && pos + i <= size;  i++) {
      cout << data[pos + i].key << " ";
    }
    pos += layerSize;
    cout << endl;
  }
}

#endif
