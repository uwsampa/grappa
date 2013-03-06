// body of the function multiMergeUnrolled
// it will be included multiple times with
// different settings for LogP
// Note that in gcc it is sufficient to simply
// use an addtional argument and to declare things an inline
// function. But I was not able to convince SunCC to
// inline and constant fold it.
// Similarly I tried introducing LogP as a template
// parameter but this did not compile on SunCC
  {
    //Entry *currentPos;
    //Key currentKey;
    //int currentIndex; // leaf pointed to by current entry
    Element *done = to + l;
    Entry    *regEntry   = entry;
    Element **regCurrent = current;
    int      winnerIndex = regEntry[0].index;
    Key      winnerKey   = regEntry[0].key;
    Element *winnerPos;
    Key sup = dummy.key; // supremum
    
    Assert2(logK >= LogK);
    while (to < done) {
      winnerPos = regCurrent[winnerIndex];
      
      // write result
      to->key   = winnerKey;
      to->value = winnerPos->value;
      
      // advance winner segment
      winnerPos++;
      regCurrent[winnerIndex] = winnerPos;
      winnerKey = winnerPos->key;
      
      // remove winner segment if empty now
      if (winnerKey == sup) { 
        deallocateSegment(winnerIndex); 
      } 
      to++;
      
      // update looser tree
#define TreeStep(L)\
      if (1 << LogK >= 1 << L) {\
        unsigned udiff = (LogK-L)+1;\
        Entry *pos##L = regEntry+((winnerIndex+(1<<LogK)) >> udiff);\
        Key    key##L = pos##L->key;\
        if (key##L < winnerKey) {\
          int index##L  = pos##L->index;\
          pos##L->key   = winnerKey;\
          pos##L->index = winnerIndex;\
          winnerKey     = key##L;\
          winnerIndex   = index##L;\
        }\
      }

      TreeStep(10);
      TreeStep(9);
      TreeStep(8);
      TreeStep(7);
      TreeStep(6);
      TreeStep(5);
      TreeStep(4);
      TreeStep(3);
      TreeStep(2);
      TreeStep(1);
#undef TreeStep      
    }
    regEntry[0].index = winnerIndex;
    regEntry[0].key   = winnerKey;  
  }
