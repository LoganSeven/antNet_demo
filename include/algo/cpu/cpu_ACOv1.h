/* Relative Path: include/algo/cpu/cpu_ACOv1.h */
/*
 * Main ACO V1 API: initialization, single/multi-ant iteration, and best-path retrieval.
 * Integrates pheromone-based route selection with an optional threaded approach.
 * Provides methods to incorporate ACO solutions into AntNet’s solver flow.
*/

#ifndef CPU_ACOV1_H
#define CPU_ACOV1_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../rendering/heatmap_renderer_api.h"

/*
 * aco_v1_init: to be implemented with actual pheromone and ant structure setups.
 * Returns 0 on success, negative on error.
 * Comment retained but adapted to reflect new logic.
 */
int aco_v1_init(AntNetContext* ctx);

/*
 * aco_v1_run_iteration: performs one iteration of the ACO logic,
 * e.g., ants traveling and updating pheromones.
 * Returns 0 on success, negative on error.
 * Comment retained but adapted to reflect new logic.
 */
int aco_v1_run_iteration(AntNetContext* ctx);

/*
 * aco_v1_get_best_path: retrieves the best path from start to end, based on pheromone trails.
 * Comment retained but adapted to reflect new logic.
 */
int aco_v1_get_best_path(
    AntNetContext* ctx,
    int* out_nodes,
    int max_size,
    int* out_path_len,
    int* out_total_latency
);

/*
 * aco_v1_search_path: replaced the former mock approach with real ACO logic
 * for demonstration in each run_all_solvers call.
 * The original mention of "mock" remains for doc continuity, but this is now a real solver.
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
