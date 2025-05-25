/* Relative Path: include/core/backend_solvers.h */
/*
 * Internal header for solver coordination and orchestration in AntNet.
 * Ties together context management with solver logic, bridging them to the public API.
 * Facilitates structured, thread-safe solver operations.
*/


#ifndef BACKEND_SOLVERS_H
#define BACKEND_SOLVERS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "backend_init.h" /* ensures priv_get_context_by_id is visible */
#include "backend.h"      /* public API definitions */

#ifdef __cplusplus
}
#endif

#endif /* BACKEND_SOLVERS_H */
