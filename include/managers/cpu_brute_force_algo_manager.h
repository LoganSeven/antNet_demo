/* Relative Path: include/managers/cpu_brute_force_algo_manager.h */
/*
 * Acts as a manager for the brute-force route search in AntNet.
 * Provides straightforward calls for initialization, running one step, and cleanup.
 * Useful for exhaustive pathfinding or validation against heuristic methods.
*/


#ifndef CPU_BRUTE_FORCE_ALGO_MANAGER_H
#define CPU_BRUTE_FORCE_ALGO_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../rendering/heatmap_renderer_api.h"

/* brute_force_algo_manager_init
 * Initializes the brute-force solver (if needed).
 * Returns 0 (ERR_SUCCESS) on success or negative error code on failure.
 */
int brute_force_algo_manager_init(AntNetContext* ctx);

/* brute_force_algo_manager_run
 * Executes a brute-force solver step for start_node -> end_node.
 * Stores results in out_nodes, out_length, out_latency.
 * Returns 0 (ERR_SUCCESS) or negative error code on failure.
 */
int brute_force_algo_manager_run(
    AntNetContext* ctx,
    int start_node,
    int end_node,
    int* out_nodes,
    int max_size,
    int* out_length,
    int* out_latency
);

/* brute_force_algo_manager_cleanup
 * Cleans up any allocated resources for the brute-force solver (if needed).
 * Returns 0 (ERR_SUCCESS) on success or negative error code on failure.
 */
int brute_force_algo_manager_cleanup(AntNetContext* ctx);

#ifdef __cplusplus
}
#endif

#endif /* CPU_BRUTE_FORCE_ALGO_MANAGER_H */
