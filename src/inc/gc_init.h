#ifndef GC_INIT_H_HEADER
#define GC_INIT_H_HEADER

struct GC_state_struct
{
  int initialized;
};

extern struct GC_state_struct GC_state;

int
GC_init (void);

int
GC_is_initialized (void);

#endif // GC_INIT_H_HEADER
