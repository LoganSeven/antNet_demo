#ifndef _STDDEF_H
#define _STDDEF_H

/* minimal C definitions for pycparser */

typedef unsigned int size_t;
typedef int          ptrdiff_t;

#define NULL ((void*)0)

/* offsetof macro – safe for parsing */
#define offsetof(type, member) ((size_t)&(((type *)0)->member))

#endif /* _STDDEF_H */
