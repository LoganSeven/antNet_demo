/* Relative Path: include/core/backend_init.h */
/*
 * backend_init.h
 * Internal header for context creation, retrieval, and shutdown logic in AntNet.
 */

#ifndef BACKEND_INIT_H
#define BACKEND_INIT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "backend.h" /* for AntNetContext, etc. */

/*
 * get_context_by_id
 * Retrieves the pointer to the context if in range and in use, else returns NULL.
 */
AntNetContext* priv_get_context_by_id(int context_id);

#ifdef __cplusplus
}
#endif

#endif /* BACKEND_INIT_H */
