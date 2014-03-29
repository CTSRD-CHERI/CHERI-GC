#ifndef REMSET_H_HEADER
#define REMSET_H_HEADER

#include <stdlib.h>

struct GC_remembered_set
{
  void ** roots;
  size_t nroots;
  size_t size; // actual size of the array (doubles each time we re-allocate)
};

void
GC_remembered_set_init (struct GC_remembered_set * remset);

// Add the address of the member of an old object which points to a young
// object, i.e., *address contains a capability to a young object. So if you
// have x.member == young_object then address should be given as &(x.member).
// Returns 0 on success and 1 on failure
int
GC_remembered_set_add (struct GC_remembered_set * remset,
                       void * address);

void
GC_remembered_set_clr (struct GC_remembered_set * remset);

#endif // REMSET_H_HEADER
