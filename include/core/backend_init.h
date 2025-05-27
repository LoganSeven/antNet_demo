/* Relative Path: include/core/backend_init.h */
/*
 * Declares internal functions for creating, retrieving, and shutting down AntNet contexts.
 * Provides priv_get_context_by_id for safely accessing a context by index.
 * Used as the backbone for managing global context arrays and concurrency.
*/


#ifndef BACKEND_INIT_H
#define BACKEND_INIT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../rendering/heatmap_renderer_api.h"

/*
 * get_context_by_id
 * Retrieves the pointer to the context if in range and in use, else returns NULL.
 */
AntNetContext* priv_get_context_by_id(int context_id);

#ifdef __cplusplus
}
#endif

#endif /* BACKEND_INIT_H */
