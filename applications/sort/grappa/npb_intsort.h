
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.


enum npb_class                  {  S,  W,  A,  B,  C,  D };
static const int NKEY_LOG2[]    = { 16, 20, 23, 25, 27, 29 };
static const int MAX_KEY_LOG2[] = { 11, 16, 19, 21, 23, 27 };
static const int NBUCKET_LOG2[] = { 10, 10, 10, 10, 10, 10 };

inline npb_class get_npb_class(char c) {
  switch (c) {
    case 'S': return S;
    case 'W': return W;
    case 'A': return A;
    case 'B': return B;
    case 'C': return C;
    case 'D': return D;
  }
}

