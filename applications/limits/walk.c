
#include "walk.h"

// walk the list
uint64_t walk( node nodes[], uint64_t count ) {
  node* i = &nodes[0];

  while (count > 0) {
    i = i->next;
    count--;
  }

  return (uint64_t)i; // give the optimizer something to think about
}

// walk the list
uint64_t multiwalk( node* bases[], 
		    uint64_t concurrent_reads, 
		    uint64_t count ) {
  node* i = bases[0];
  node* j = concurrent_reads >=  2 ? bases[1] : NULL;
  node* k = concurrent_reads >=  3 ? bases[2] : NULL;
  node* l = concurrent_reads >=  4 ? bases[3] : NULL;
  node* m = concurrent_reads >=  5 ? bases[4] : NULL;
  node* n = concurrent_reads >=  6 ? bases[5] : NULL;
  node* o = concurrent_reads >=  7 ? bases[6] : NULL;
  node* p = concurrent_reads >=  8 ? bases[7] : NULL;
  node* q = concurrent_reads >=  9 ? bases[8] : NULL;
  node* r = concurrent_reads >= 10 ? bases[9] : NULL;
  node* s = concurrent_reads >= 11 ? bases[10] : NULL;
  node* t = concurrent_reads >= 12 ? bases[11] : NULL;
  
  switch (concurrent_reads) {
  case 1: 
    {
      while (count > 0) {
	i = i->next;
	count--;
      }
    }
    break;

  case 2: 
    {
      while (count > 0) {
	i = i->next;
	j = j->next;
	count--;
      }
    }
    break;

  case 3: 
    {
      while (count > 0) {
	i = i->next;
	j = j->next;
	k = k->next;
	count--;
      }
    }
    break;

  case 4: 
    {
      while (count > 0) {
	i = i->next;
	j = j->next;
	k = k->next;
	l = l->next;
	count--;
      }
    }
    break;


  case 5: 
    {
      while (count > 0) {
	i = i->next;
	j = j->next;
	k = k->next;
	l = l->next;
	m = m->next;
	count--;
      }
    }
    break;


  case 6: 
    {
      while (count > 0) {
	i = i->next;
	j = j->next;
	k = k->next;
	l = l->next;
	m = m->next;
	n = n->next;
	count--;
      }
    }
    break;


  case 7: 
    {
      while (count > 0) {
	i = i->next;
	j = j->next;
	k = k->next;
	l = l->next;
	m = m->next;
	n = n->next;
	o = o->next;
	count--;
      }
    }
    break;

  case 8: 
    {
      while (count > 0) {
	i = i->next;
	j = j->next;
	k = k->next;
	l = l->next;
	m = m->next;
	n = n->next;
	o = o->next;
	p = p->next;
	count--;
      }
    }
    break;

  case 9: 
    {
      while (count > 0) {
	i = i->next;
	j = j->next;
	k = k->next;
	l = l->next;
	m = m->next;
	n = n->next;
	o = o->next;
	p = p->next;
	q = q->next;
	count--;
      }
    }
    break;

  case 10: 
    {
      while (count > 0) {
	i = i->next;
	j = j->next;
	k = k->next;
	l = l->next;
	m = m->next;
	n = n->next;
	o = o->next;
	p = p->next;
	q = q->next;
	r = r->next;
	count--;
      }
    }
    break;

  case 11: 
    {
      while (count > 0) {
	i = i->next;
	j = j->next;
	k = k->next;
	l = l->next;
	m = m->next;
	n = n->next;
	o = o->next;
	p = p->next;
	q = q->next;
	r = r->next;
	s = s->next;
	count--;
      }
    }
    break;

  case 12: 
    {
      while (count > 0) {
	i = i->next;
	j = j->next;
	k = k->next;
	l = l->next;
	m = m->next;
	n = n->next;
	o = o->next;
	p = p->next;
	q = q->next;
	r = r->next;
	s = s->next;
	t = t->next;
	count--;
      }
    }
    break;


    }

  return (uint64_t)i
    + (uint64_t)j
    + (uint64_t)k
    + (uint64_t)l
    + (uint64_t)m
    + (uint64_t)n
    + (uint64_t)o
    + (uint64_t)p
    + (uint64_t)q
    + (uint64_t)r
    + (uint64_t)s
    + (uint64_t)t;
}
