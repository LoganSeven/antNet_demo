#ifndef _PTHREAD_STUBS_H
#define _PTHREAD_STUBS_H

/* Basic thread types */
typedef unsigned long pthread_t;        /* Thread ID (opaque handle) */
typedef unsigned int  pthread_key_t;    /* Thread-local storage key */
typedef int           pthread_once_t;   /* One-time initializer flag */

/* Opaque structures for thread attributes and sync primitives */
typedef struct __pthread_attr_t      { char __size[56]; } pthread_attr_t;
typedef struct __pthread_mutex_t     { char __size[40]; } pthread_mutex_t;
typedef struct __pthread_mutexattr_t { char __size[4];  } pthread_mutexattr_t;
typedef struct __pthread_cond_t      { char __size[48]; } pthread_cond_t;
typedef struct __pthread_condattr_t  { char __size[4];  } pthread_condattr_t;
typedef struct __pthread_rwlock_t    { char __size[56]; } pthread_rwlock_t;
typedef struct __pthread_rwlockattr_t{ char __size[8];  } pthread_rwlockattr_t;
typedef struct __pthread_barrier_t   { char __size[32]; } pthread_barrier_t;
typedef struct __pthread_barrierattr_t{ char __size[4]; } pthread_barrierattr_t;
typedef volatile int pthread_spinlock_t;  /* spinlock (usually an int) */

#endif  /* _PTHREAD_STUBS_H */
