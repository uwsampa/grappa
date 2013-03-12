
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

enum NPBClass               {  S,  W,  A,  B,  C,  D };
static const int NKEY_LOG2[]    = { 16, 20, 23, 25, 27, 31 };
static const int MAX_KEY_LOG2[] = { 11, 16, 19, 21, 23, 27 };
static const int NBUCKET_LOG2[] = { 10, 10, 10, 10, 10, 10 };

inline NPBClass get_npb_class(char c) {
  switch (c) {
    case 'S': return NPBClass::S;
    case 'W': return NPBClass::W;
    case 'A': return NPBClass::A;
    case 'B': return NPBClass::B;
    case 'C': return NPBClass::C;
    case 'D': return NPBClass::D;
  }
}
inline char npb_class_char(NPBClass c) {
  switch (c) {
    case NPBClass::S: return 'S';
    case NPBClass::W: return 'W';
    case NPBClass::A: return 'A';
    case NPBClass::B: return 'B';
    case NPBClass::C: return 'C';
    case NPBClass::D: return 'D';
  }
}
