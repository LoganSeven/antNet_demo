/* Relative Path: include/managers/cpu_random_algo_manager.h */
/*
 * Coordinates a simple random solver that picks nodes and forms paths arbitrarily.
 * Exposes methods to initialize, run, and clean up random-based pathfinding.
 * Suitable as a baseline or comparison solver in AntNet.
*/


#ifndef CPU_RANDOM_ALGO_MANAGER_H
#define CPU_RANDOM_ALGO_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../core/backend.h"

/* random_algo_manager_init
 * Initializes the random solver (if needed).
 * Returns 0 (ERR_SUCCESS) on success, or negative error code on failure.
 */
int random_algo_manager_init(AntNetContext* ctx);

/* random_algo_manager_run
 * Executes the random solver for start_node -> end_node.
 * Stores results in out_nodes, out_length, out_latency.
 * Returns 0 (ERR_SUCCESS) or negative error code on failure.
 */
int random_algo_manager_run(
    AntNetContext* ctx,
    int start_node,
    int end_node,
    int* out_nodes,
    int max_size,
    int* out_length,
    int* out_latency
);

/* random_algo_manager_cleanup
 * Cleans up any allocated resources for the random solver (if needed).
 * Returns 0 (ERR_SUCCESS) on success, or negative error code on failure.
 */
int random_algo_manager_cleanup(AntNetContext* ctx);

#ifdef __cplusplus
}
#endif

#endif /* CPU_RANDOM_ALGO_MANAGER_H */
