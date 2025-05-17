/* Relative Path: include/core/backend_thread_defs.h */
#ifndef BACKEND_THREAD_DEFS_H
#define BACKEND_THREAD_DEFS_H

#ifndef CFFI_BUILD

#ifndef _WIN32
#include <pthread.h>
#else
typedef void* pthread_mutex_t; /* Windows minimal placeholder */
#endif

#else /* CFFI_BUILD */

/* Explicitly include stub pthread definition for CFFI */
#include "../stub_headers/pthread.h"

#endif /* CFFI_BUILD */

#endif /* BACKEND_THREAD_DEFS_H */
