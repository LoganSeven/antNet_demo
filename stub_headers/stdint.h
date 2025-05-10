#ifndef _STDINT_H
#define _STDINT_H

/* fixed-width integer typedefs – minimal for pycparser parsing */

typedef signed   char  int8_t;
typedef unsigned char  uint8_t;

typedef short           int16_t;
typedef unsigned short  uint16_t;

typedef int             int32_t;
typedef unsigned int    uint32_t;

/* long types cover both LLP64 and LP64 for parsing purposes */
typedef long long          int64_t;
typedef unsigned long long uint64_t;

/* intptr and uintptr (pointer-sized) */
typedef long             intptr_t;
typedef unsigned long    uintptr_t;

/* size and ptrdiff already in stddef.h stub, but include guards prevent redefinition */

#endif /* _STDINT_H */
