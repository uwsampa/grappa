#include "knheap.h"
#include <string.h>
///////////////////////// LooserTree ///////////////////////////////////
template <class Key, class Value>
KNLooserTree<Key, Value>::
KNLooserTree() : lastFree(0), size(0), logK(0), k(1)
{
  empty  [0] = 0;
  segment[0] = 0;
  current[0] = &dummy;
  // entry and dummy are initialized by init
  // since they need the value of supremum
}


template <class Key, class Value>
void KNLooserTree<Key, Value>::
init(Key sup)
{
  dummy.key      = sup;
  rebuildLooserTree();
  Assert2(current[entry[0].index] == &dummy);
}


// rebuild looser tree information from the values in current
template <class Key, class Value>
void KNLooserTree<Key, Value>::
rebuildLooserTree()
{  
  int winner = initWinner(1);
  entry[0].index = winner;
  entry[0].key   = current[winner]->key;
}


// given any values in the leaves this
// routing recomputes upper levels of the tree
// from scratch in linear time
// initialize entry[root].index and the subtree rooted there
// return winner index
template <class Key, class Value>
int KNLooserTree<Key, Value>::
initWinner(int root)
{
  if (root >= k) { // leaf reached
    return root - k;
  } else {
    int left  = initWinner(2*root    );
    int right = initWinner(2*root + 1);
    Key lk    = current[left ]->key;
    Key rk    = current[right]->key;
    if (lk <= rk) { // right subtree looses
      entry[root].index = right;
      entry[root].key   = rk;
      return left;
    } else {
      entry[root].index = left;
      entry[root].key   = lk;
      return right;
    }
  }
}


// first go up the tree all the way to the root
// hand down old winner for the respective subtree
// based on new value, and old winner and looser 
// update each node on the path to the root top down.
// This is implemented recursively
template <class Key, class Value>
void KNLooserTree<Key, Value>::
updateOnInsert(int node, 
               Key     newKey, int     newIndex, 
               Key *winnerKey, int *winnerIndex, // old winner
               int *mask) // 1 << (ceil(log KNK) - dist-from-root)
{
  if (node == 0) { // winner part of root
    *mask = 1 << (logK - 1);    
    *winnerKey   = entry[0].key;
    *winnerIndex = entry[0].index;
    if (newKey < entry[node].key) {
      entry[node].key   = newKey;
      entry[node].index = newIndex;
    }
  } else {
    updateOnInsert(node >> 1, newKey, newIndex, winnerKey, winnerIndex, mask);
    Key looserKey   = entry[node].key;
    int looserIndex = entry[node].index;
    if ((*winnerIndex & *mask) != (newIndex & *mask)) { // different subtrees
      if (newKey < looserKey) { // newKey will have influence here
        if (newKey < *winnerKey) { // old winner loses here
          entry[node].key   = *winnerKey;
          entry[node].index = *winnerIndex;
        } else { // new entry looses here
          entry[node].key   = newKey;
          entry[node].index = newIndex;
        }
      } 
      *winnerKey   = looserKey;
      *winnerIndex = looserIndex;
    }
    // note that nothing needs to be done if
    // the winner came from the same subtree
    // a) newKey <= winnerKey => even more reason for the other tree to loose
    // b) newKey >  winnerKey => the old winner will beat the new
    //                           entry further down the tree
    // also the same old winner is handed down the tree

    *mask >>= 1; // next level
  }
}


// make the tree two times as wide
// may only be called if no free slots are left ?? necessary ??
template <class Key, class Value>
void KNLooserTree<Key, Value>::
doubleK()
{
  // make all new entries empty
  // and push them on the free stack
  Assert2(lastFree == -1); // stack was empty (probably not needed)
  Assert2(k < KNKMAX);
  for (int i = 2*k - 1;  i >= k;  i--) {
    current[i] = &dummy;
    lastFree++;
    empty[lastFree] = i;
  }

  // double the size
  k *= 2;  logK++;

  // recompute looser tree information
  rebuildLooserTree();
}


// compact nonempty segments in the left half of the tree
template <class Key, class Value>
void KNLooserTree<Key, Value>::
compactTree()
{
  Assert2(logK > 0);
  Key sup = dummy.key;

  // compact all nonempty segments to the left
  int from = 0;
  int to   = 0;
  for(;  from < k;  from++) {
    if (current[from]->key != sup) {
      current[to] = current[from];
      segment[to] = segment[from];
      to++;
    } 
  }

  // half degree as often as possible
  while (to < k/2) {
    k /= 2;  logK--;
  }

  // overwrite garbage and compact the stack of empty segments
  lastFree = -1; // none free
  for (;  to < k;  to++) {
    // push 
    lastFree++;
    empty[lastFree] = to;

    current[to] = &dummy;
  }

  // recompute looser tree information
  rebuildLooserTree();
}


// insert segment beginning at to
// require: spaceIsAvailable() == 1 
template <class Key, class Value>
void KNLooserTree<Key, Value>::
insertSegment(Element *to, int sz)
{
  if (sz > 0) {
    Assert2(to[0   ].key != getSupremum());
    Assert2(to[sz-1].key != getSupremum());
    // get a free slot
    if (lastFree < 0) { // tree is too small
      doubleK();
    }
    int index = empty[lastFree];
    lastFree--; // pop


    // link new segment
    current[index] = segment[index] = to;
    size += sz;
    
    // propagate new information up the tree
    Key dummyKey;
    int dummyIndex;
    int dummyMask;
    updateOnInsert((index + k) >> 1, to->key, index, 
                   &dummyKey, &dummyIndex, &dummyMask);
  } else {
    // immediately deallocate
    // this is not only an optimization 
    // but also needed to keep empty segments from
    // clogging up the tree
    delete [] to;
  }
}


// free an empty segment
template <class Key, class Value>
void KNLooserTree<Key, Value>::
deallocateSegment(int index)
{
  // reroute current pointer to some empty dummy segment
  // with a sentinel key
  current[index] = &dummy;

  // free memory
  delete [] segment[index];
  segment[index] = 0;
  
  // push on the stack of free segment indices
  lastFree++;
  empty[lastFree] = index;
}

// multi-merge for a fixed K=1<<LogK
// this looks ugly but the include file explains
// why this is the most portable choice
#define LogK 3
template <class Key, class Value>
void KNLooserTree<Key, Value>::
multiMergeUnrolled3(Element *to, int l)
#include "multiMergeUnrolled.C"
#undef LogK
#define LogK 4
template <class Key, class Value>
void KNLooserTree<Key, Value>::
multiMergeUnrolled4(Element *to, int l)
#include "multiMergeUnrolled.C"
#undef LogK
#define LogK 5
template <class Key, class Value>
void KNLooserTree<Key, Value>::
multiMergeUnrolled5(Element *to, int l)
#include "multiMergeUnrolled.C"
#undef LogK
#define LogK 6
template <class Key, class Value>
void KNLooserTree<Key, Value>::
multiMergeUnrolled6(Element *to, int l)
#include "multiMergeUnrolled.C"
#undef LogK
#define LogK 7
template <class Key, class Value>
void KNLooserTree<Key, Value>::
multiMergeUnrolled7(Element *to, int l)
#include "multiMergeUnrolled.C"
#undef LogK
#define LogK 8
template <class Key, class Value>
void KNLooserTree<Key, Value>::
multiMergeUnrolled8(Element *to, int l)
#include "multiMergeUnrolled.C"
#undef LogK
#define LogK 9
template <class Key, class Value>
void KNLooserTree<Key, Value>::
multiMergeUnrolled9(Element *to, int l)
#include "multiMergeUnrolled.C"
#undef LogK
#define LogK 10
template <class Key, class Value>
void KNLooserTree<Key, Value>::
multiMergeUnrolled10(Element *to, int l)
#include "multiMergeUnrolled.C"
#undef LogK

// delete the l smallest elements and write them to "to"
// empty segments are deallocated
// require:
// - there are at least l elements
// - segments are ended by sentinels
template <class Key, class Value>
void KNLooserTree<Key, Value>::
multiMerge(Element *to, int l)
{
  switch(logK) {
  case 0: 
    Assert2(k == 1);
    Assert2(entry[0].index == 0);
    Assert2(lastFree == -1 || l == 0);
    memcpy(to, current[0], l * sizeof(Element));
    current[0] += l;
    entry[0].key = current[0]->key;
    if (segmentIsEmpty(0)) deallocateSegment(0); 
    break;
  case 1:
    Assert2(k == 2);
    merge(current + 0, current + 1, to, l);
    rebuildLooserTree();
    if (segmentIsEmpty(0)) deallocateSegment(0); 
    if (segmentIsEmpty(1)) deallocateSegment(1); 
    break;
  case 2:
    Assert2(k == 4);
    merge4(current + 0, current + 1, current + 2, current + 3, to, l);
    rebuildLooserTree();
    if (segmentIsEmpty(0)) deallocateSegment(0); 
    if (segmentIsEmpty(1)) deallocateSegment(1); 
    if (segmentIsEmpty(2)) deallocateSegment(2); 
    if (segmentIsEmpty(3)) deallocateSegment(3);
    break;
  case  3: multiMergeUnrolled3(to, l); break;
  case  4: multiMergeUnrolled4(to, l); break;
  case  5: multiMergeUnrolled5(to, l); break;
  case  6: multiMergeUnrolled6(to, l); break;
  case  7: multiMergeUnrolled7(to, l); break;
  case  8: multiMergeUnrolled8(to, l); break;
  case  9: multiMergeUnrolled9(to, l); break;
  case 10: multiMergeUnrolled10(to, l); break;
  default: multiMergeK       (to, l); break;
  }
  size -= l;

  // compact tree if it got considerably smaller
  if (k > 1 && lastFree >= 3*k/5 - 1) { 
    // using k/2 would be worst case inefficient
    compactTree(); 
  }
}


// is this segment empty and does not point to dummy yet?
template <class Key, class Value>
inline int KNLooserTree<Key, Value>::
segmentIsEmpty(int i)
{
  return current[i]->key == getSupremum() && 
         current[i] != &dummy;
}


// multi-merge for arbitrary K
template <class Key, class Value>
void KNLooserTree<Key, Value>::
multiMergeK(Element *to, int l)
{
  Entry *currentPos;
  Key currentKey;
  int currentIndex; // leaf pointed to by current entry
  int kReg = k;
  Element *done = to + l;
  int      winnerIndex = entry[0].index;
  Key      winnerKey   = entry[0].key;
  Element *winnerPos;
  Key sup = dummy.key; // supremum
  while (to < done) {
    winnerPos = current[winnerIndex];

    // write result
    to->key   = winnerKey;
    to->value = winnerPos->value;

    // advance winner segment
    winnerPos++;
    current[winnerIndex] = winnerPos;
    winnerKey = winnerPos->key;

    // remove winner segment if empty now
    if (winnerKey == sup) { 
      deallocateSegment(winnerIndex); 
    } 
    
    // go up the entry-tree
    for (int i = (winnerIndex + kReg) >> 1;  i > 0;  i >>= 1) {
      currentPos = entry + i;
      currentKey = currentPos->key;
      if (currentKey < winnerKey) {
        currentIndex      = currentPos->index;
        currentPos->key   = winnerKey;
        currentPos->index = winnerIndex;
        winnerKey         = currentKey;
        winnerIndex       = currentIndex;
      }
    }

    to++;
  }
  entry[0].index = winnerIndex;
  entry[0].key   = winnerKey;  
}

////////////////////////// KNHeap //////////////////////////////////////
template <class Key, class Value>
KNHeap<Key, Value>::
KNHeap(Key sup, Key infimum) : insertHeap(sup, infimum),
                                   activeLevels(0), size(0)
{
  buffer1[KNBufferSize1].key = sup; // sentinel
  minBuffer1 = buffer1 + KNBufferSize1; // empty
  for (int i = 0;  i < KNLevels;  i++) {
    tree[i].init(sup); // put tree[i] in a consistent state
    buffer2[i][KNN].key = sup; // sentinel
    minBuffer2[i] = &(buffer2[i][KNN]); // empty
  }
}


//--------------------- Buffer refilling -------------------------------

// refill buffer2[j] and return number of elements found
template <class Key, class Value>
int KNHeap<Key, Value>::refillBuffer2(int j)
{
  Element *oldTarget;
  int deleteSize;
  int treeSize = tree[j].getSize();
  int bufferSize = (&(buffer2[j][0]) + KNN) - minBuffer2[j];
  if (treeSize + bufferSize >= KNN) { // buffer will be filled
    oldTarget = &(buffer2[j][0]);
    deleteSize = KNN - bufferSize;
  } else {
    oldTarget = &(buffer2[j][0]) + KNN - treeSize - bufferSize;
    deleteSize = treeSize;
  }

  // shift  rest to beginning
  // possible hack:
  // - use memcpy if no overlap
  memmove(oldTarget, minBuffer2[j], bufferSize * sizeof(Element));
  minBuffer2[j] = oldTarget;

  // fill remaining space from tree
  tree[j].multiMerge(oldTarget + bufferSize, deleteSize);
  return deleteSize + bufferSize;
}
 
 
// move elements from the 2nd level buffers 
// to the delete buffer
template <class Key, class Value>
void KNHeap<Key, Value>::refillBuffer1() 
{
  int totalSize = 0;
  int sz;
  for (int i = activeLevels - 1;  i >= 0;  i--) {
    if ((&(buffer2[i][0]) + KNN) - minBuffer2[i] < KNBufferSize1) {
      sz = refillBuffer2(i);
      // max active level dry now?
      if (sz == 0 && i == activeLevels - 1) { activeLevels--; }
      else {totalSize += sz;}
    } else {
      totalSize += KNBufferSize1; // actually only a sufficient lower bound
    }
  }
  if (totalSize >= KNBufferSize1) { // buffer can be filled
    minBuffer1 = buffer1;
    sz = KNBufferSize1; // amount to be copied
    size -= KNBufferSize1; // amount left in buffer2
  } else {
    minBuffer1 = buffer1 + KNBufferSize1 - totalSize;
    sz = totalSize;
    Assert2(size == sz); // trees and buffer2 get empty
    size = 0;
  }

  // now call simplified refill routines
  // which can make the assumption that
  // they find all they are asked to find in the buffers
  minBuffer1 = buffer1 + KNBufferSize1 - sz;
  switch(activeLevels) {
  case 1: memcpy(minBuffer1, minBuffer2[0], sz * sizeof(Element));
          minBuffer2[0] += sz;
          break;
  case 2: merge(&(minBuffer2[0]), 
                &(minBuffer2[1]), minBuffer1, sz);
          break;
  case 3: merge3(&(minBuffer2[0]), 
                 &(minBuffer2[1]),
                 &(minBuffer2[2]), minBuffer1, sz);
          break;
  case 4: merge4(&(minBuffer2[0]), 
                 &(minBuffer2[1]),
                 &(minBuffer2[2]),
                 &(minBuffer2[3]), minBuffer1, sz);
          break;
//  case 2: refillBuffer12(sz); break;
//  case 3: refillBuffer13(sz); break;
//  case 4: refillBuffer14(sz); break;
  }
}


template <class Key, class Value>
void KNHeap<Key, Value>::refillBuffer13(int sz) 
{ Assert(0); // not yet implemented
}

template <class Key, class Value>
void KNHeap<Key, Value>::refillBuffer14(int sz) 
{ Assert(0); // not yet implemented
}


//--------------------------------------------------------------------

// check if space is available on level k and
// empty this level if necessary leading to a recursive call.
// return the level where space was finally available
template <class Key, class Value>
int KNHeap<Key, Value>::makeSpaceAvailable(int level)
{
  int finalLevel;

  Assert2(level <= activeLevels);
  if (level == activeLevels) { activeLevels++; }
  if (tree[level].spaceIsAvailable()) { 
    finalLevel = level;
  } else {
    finalLevel = makeSpaceAvailable(level + 1);
    int segmentSize = tree[level].getSize();
    Element *newSegment = new Element[segmentSize + 1];

    tree[level].multiMerge(newSegment, segmentSize); // empty this level
    //    tree[level].cleanUp();
    newSegment[segmentSize].key = buffer1[KNBufferSize1].key; // sentinel
    // for queues where size << #inserts
    // it might make sense to stay in this level if
    // segmentSize < alpha * KNN * k^level for some alpha < 1
    tree[level + 1].insertSegment(newSegment, segmentSize);
  }
  return finalLevel;
}


// empty the insert heap into the main data structure
template <class Key, class Value>
void KNHeap<Key, Value>::emptyInsertHeap()
{
  const Key sup = getSupremum();

  // build new segment
  Element *newSegment = new Element[KNN + 1];
  Element *newPos = newSegment;

  // put the new data there for now
  insertHeap.sortTo(newSegment);
  newSegment[KNN].key = sup; // sentinel

  // copy the buffer1 and buffer2[0] to temporary storage
  // (the tomporary can be eliminated using some dirty tricks)
  const int tempSize = KNN + KNBufferSize1;
  Element temp[tempSize + 1]; 
  int sz1 = getSize1();
  int sz2 = getSize2(0);
  Element *pos = temp + tempSize - sz1 - sz2;
  memcpy(pos      , minBuffer1   , sz1 * sizeof(Element)); 
  memcpy(pos + sz1, minBuffer2[0], sz2 * sizeof(Element));
  temp[tempSize].key = sup; // sentinel

  // refill buffer1
  // (using more complicated code it could be made somewhat fuller
  // in certein circumstances)
  merge(&pos, &newPos, minBuffer1, sz1);

  // refill buffer2[0]
  // (as above we might want to take the opportunity
  // to make buffer2[0] fuller)
  merge(&pos, &newPos, minBuffer2[0], sz2);

  // merge the rest to the new segment
  // note that merge exactly trips into the footsteps
  // of itself
  merge(&pos, &newPos, newSegment, KNN);
  
  // and insert it
  int freeLevel = makeSpaceAvailable(0);
  Assert2(freeLevel == 0 || tree[0].getSize() == 0);
  tree[0].insertSegment(newSegment, KNN);

  // get rid of invalid level 2 buffers
  // by inserting them into tree 0 (which is almost empty in this case)
  if (freeLevel > 0) {
    for (int i = freeLevel;  i >= 0;  i--) { // reverse order not needed 
      // but would allow immediate refill
      newSegment = new Element[getSize2(i) + 1]; // with sentinel
      memcpy(newSegment, minBuffer2[i], (getSize2(i) + 1) * sizeof(Element));
      tree[0].insertSegment(newSegment, getSize2(i));
      minBuffer2[i] = buffer2[i] + KNN; // empty
    }
  }

  // update size
  size += KNN; 

  // special case if the tree was empty before
  if (minBuffer1 == buffer1 + KNBufferSize1) { refillBuffer1(); }
}

/////////////////////////////////////////////////////////////////////
// auxiliary functions

// merge sz element from the two sentinel terminated input
// sequences *f0 and *f1 to "to"
// advance *fo and *f1 accordingly.
// require: at least sz nonsentinel elements available in f0, f1
// require: to may overwrite one of the sources as long as
//   *fx + sz is before the end of fx
template <class Key, class Value>
void merge(KNElement<Key, Value> **f0,
           KNElement<Key, Value> **f1,
           KNElement<Key, Value>  *to, int sz) 
{
  KNElement<Key, Value> *from0   = *f0;
  KNElement<Key, Value> *from1   = *f1;
  KNElement<Key, Value> *done    = to + sz;
  Key      key0    = from0->key;
  Key      key1    = from1->key;

  while (to < done) {
    if (key1 <= key0) {
      to->key   = key1;
      to->value = from1->value; // note that this may be the same address
      from1++; // nach hinten schieben?
      key1 = from1->key;
    } else {
      to->key   = key0;
      to->value = from0->value; // note that this may be the same address
      from0++; // nach hinten schieben?
      key0 = from0->key;
    }
    to++;
  }
  *f0   = from0;
  *f1   = from1;
}


// merge sz element from the three sentinel terminated input
// sequences *f0, *f1 and *f2 to "to"
// advance *f0, *f1 and *f2 accordingly.
// require: at least sz nonsentinel elements available in f0, f1 and f2
// require: to may overwrite one of the sources as long as
//   *fx + sz is before the end of fx
template <class Key, class Value>
void merge3(KNElement<Key, Value> **f0,
           KNElement<Key, Value> **f1,
           KNElement<Key, Value> **f2,
           KNElement<Key, Value>  *to, int sz) 
{
  KNElement<Key, Value> *from0   = *f0;
  KNElement<Key, Value> *from1   = *f1;
  KNElement<Key, Value> *from2   = *f2;
  KNElement<Key, Value> *done    = to + sz;
  Key      key0    = from0->key;
  Key      key1    = from1->key;
  Key      key2    = from2->key;

  if (key0 < key1) {
    if (key1 < key2)   { goto s012; }
    else { 
      if (key2 < key0) { goto s201; }
      else             { goto s021; }
    }
  } else {
    if (key1 < key2) {
      if (key0 < key2) { goto s102; }
      else             { goto s120; }
    } else             { goto s210; }
  }

#define Merge3Case(a,b,c)\
  s ## a ## b ## c :\
  if (to == done) goto finish;\
  to->key = key ## a;\
  to->value = from ## a -> value;\
  to++;\
  from ## a ++;\
  key ## a = from ## a -> key;\
  if (key ## a < key ## b) goto s ## a ## b ## c;\
  if (key ## a < key ## c) goto s ## b ## a ## c;\
  goto s ## b ## c ## a;

  // the order is choosen in such a way that 
  // four of the trailing gotos can be eliminated by the optimizer
  Merge3Case(0, 1, 2);
  Merge3Case(1, 2, 0);
  Merge3Case(2, 0, 1);
  Merge3Case(1, 0, 2);
  Merge3Case(0, 2, 1);
  Merge3Case(2, 1, 0);

 finish:
  *f0   = from0;
  *f1   = from1;
  *f2   = from2;
}


// merge sz element from the three sentinel terminated input
// sequences *f0, *f1, *f2 and *f3 to "to"
// advance *f0, *f1, *f2 and *f3 accordingly.
// require: at least sz nonsentinel elements available in f0, f1, f2 and f2
// require: to may overwrite one of the sources as long as
//   *fx + sz is before the end of fx
template <class Key, class Value>
void merge4(KNElement<Key, Value> **f0,
           KNElement<Key, Value> **f1,
           KNElement<Key, Value> **f2,
           KNElement<Key, Value> **f3,
           KNElement<Key, Value>  *to, int sz) 
{
  KNElement<Key, Value> *from0   = *f0;
  KNElement<Key, Value> *from1   = *f1;
  KNElement<Key, Value> *from2   = *f2;
  KNElement<Key, Value> *from3   = *f3;
  KNElement<Key, Value> *done    = to + sz;
  Key      key0    = from0->key;
  Key      key1    = from1->key;
  Key      key2    = from2->key;
  Key      key3    = from3->key;

#define StartMerge4(a, b, c, d)\
  if (key##a <= key##b && key##b <= key##c && key##c <= key##d)\
    goto s ## a ## b ## c ## d;

  StartMerge4(0, 1, 2, 3);
  StartMerge4(1, 2, 3, 0);
  StartMerge4(2, 3, 0, 1);
  StartMerge4(3, 0, 1, 2);

  StartMerge4(0, 3, 1, 2);
  StartMerge4(3, 1, 2, 0);
  StartMerge4(1, 2, 0, 3);
  StartMerge4(2, 0, 3, 1);

  StartMerge4(0, 2, 3, 1);
  StartMerge4(2, 3, 1, 0);
  StartMerge4(3, 1, 0, 2);
  StartMerge4(1, 0, 2, 3);

  StartMerge4(2, 0, 1, 3);
  StartMerge4(0, 1, 3, 2);
  StartMerge4(1, 3, 2, 0);
  StartMerge4(3, 2, 0, 1);

  StartMerge4(3, 0, 2, 1);
  StartMerge4(0, 2, 1, 3);
  StartMerge4(2, 1, 3, 0);
  StartMerge4(1, 3, 0, 2);

  StartMerge4(1, 0, 3, 2);
  StartMerge4(0, 3, 2, 1);
  StartMerge4(3, 2, 1, 0);
  StartMerge4(2, 1, 0, 3);

#define Merge4Case(a, b, c, d)\
  s ## a ## b ## c ## d:\
  if (to == done) goto finish;\
  to->key = key ## a;\
  to->value = from ## a -> value;\
  to++;\
  from ## a ++;\
  key ## a = from ## a -> key;\
  if (key ## a < key ## c) {\
    if (key ## a < key ## b) { goto s ## a ## b ## c ## d; }\
    else                     { goto s ## b ## a ## c ## d; }\
  } else {\
    if (key ## a < key ## d) { goto s ## b ## c ## a ## d; }\
    else                     { goto s ## b ## c ## d ## a; }\
  }    
  
  Merge4Case(0, 1, 2, 3);
  Merge4Case(1, 2, 3, 0);
  Merge4Case(2, 3, 0, 1);
  Merge4Case(3, 0, 1, 2);

  Merge4Case(0, 3, 1, 2);
  Merge4Case(3, 1, 2, 0);
  Merge4Case(1, 2, 0, 3);
  Merge4Case(2, 0, 3, 1);

  Merge4Case(0, 2, 3, 1);
  Merge4Case(2, 3, 1, 0);
  Merge4Case(3, 1, 0, 2);
  Merge4Case(1, 0, 2, 3);

  Merge4Case(2, 0, 1, 3);
  Merge4Case(0, 1, 3, 2);
  Merge4Case(1, 3, 2, 0);
  Merge4Case(3, 2, 0, 1);

  Merge4Case(3, 0, 2, 1);
  Merge4Case(0, 2, 1, 3);
  Merge4Case(2, 1, 3, 0);
  Merge4Case(1, 3, 0, 2);

  Merge4Case(1, 0, 3, 2);
  Merge4Case(0, 3, 2, 1);
  Merge4Case(3, 2, 1, 0);
  Merge4Case(2, 1, 0, 3);

 finish:
  *f0   = from0;
  *f1   = from1;
  *f2   = from2;
  *f3   = from3;
}
