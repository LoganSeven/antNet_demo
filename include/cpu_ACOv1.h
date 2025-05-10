// include/cpu_ACOv1.h
#ifndef CPU_ACOV1_H
#define CPU_ACOV1_H

/*
 * cpu_ACOv1.h
 * Skeleton for a future version 1 ACO-based solver.
 * This file declares placeholders for initialization, iteration, and best-path retrieval.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "backend.h"

/*
 * aco_v1_init: to be implemented with actual pheromone and ant structure setups.
 * Returns 0 on success, negative on error.
 */
int aco_v1_init(AntNetContext* ctx);

/*
 * aco_v1_run_iteration: performs one iteration of the ACO logic,
 * e.g., ants traveling and updating pheromones.
 * Returns 0 on success, negative on error.
 */
int aco_v1_run_iteration(AntNetContext* ctx);

/*
 * aco_v1_get_best_path: retrieves the best path from start to end, based on pheromone trails.
 * For now, it may remain unimplemented or stubbed out.
 */
int aco_v1_get_best_path(
    AntNetContext* ctx,
    int* out_nodes,
    int max_size,
    int* out_path_len,
    int* out_total_latency
);

/*
 * aco_v1_search_path: placeholder path-finding method. Currently a mock approach.
 * This function will be replaced later by actual ACO logic.
 */
int aco_v1_search_path(
    AntNetContext* ctx,
    int start_id,
    int end_id,
    int* out_nodes,
    int max_size,
    int* out_path_len,
    int* out_total_latency
);

#ifdef __cplusplus
}
#endif

#endif // CPU_ACOV1_H
