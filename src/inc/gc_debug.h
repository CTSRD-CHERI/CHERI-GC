#ifndef GC_DEBUG_H_HEADER
#define GC_DEBUG_H_HEADER

#define GC_dbgf(...)  GC_dbgf2(__FILE__, __LINE__, __VA_ARGS__)

void
GC_dbgf2 (const char * file, int line, const char * format, ...);

#endif // GC_DEBUG_H_HEADER
