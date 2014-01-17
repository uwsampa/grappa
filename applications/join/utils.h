#pragma once

uint64_t identity_hash( int64_t k ) {
  return k;
}

template <typename D, typename S1, typename S2>
D combine(S1 s1, S2 s2) {
  D d;
  for (int i=0; i<s1.numFields(); i++) {
    d.set(i, s1.get(i));
  }
  for (int i=0; i<s2.numFields(); i++) {
    d.set(s1.numFields()+i, s2.get(i));
  }
  return d;
}
