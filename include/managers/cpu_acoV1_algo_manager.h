/* Relative Path: include/managers/cpu_acoV1_algo_manager.h */
/*
 * cpu_aco_algo_manager.h
 * Manager for the ACO-based search algorithm in AntNet.
 */

#ifndef CPU_ACOV1_ALGO_MANAGER_H
#define CPU_ACOV1_ALGO_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../core/backend.h"

/* aco_algo_manager_init
 * Initializes the ACO solver (if needed).
 * Returns 0 (ERR_SUCCESS) on success or negative error code on failure.
 */
int aco_algo_manager_init(AntNetContext* ctx);

/* aco_algo_manager_run_iteration
 * Performs one ACO solver iteration.
 * Returns 0 (ERR_SUCCESS) or negative error code on failure.
 */
int aco_algo_manager_run_iteration(AntNetContext* ctx);

/* aco_algo_manager_search_path
 * Searches for a path from start_node to end_node using ACO data.
 * Stores results in out_nodes, out_length, out_latency.
 * Returns 0 (ERR_SUCCESS) or negative error code on failure.
 */
int aco_algo_manager_search_path(
    AntNetContext* ctx,
    int start_node,
    int end_node,
    int* out_nodes,
    int max_size,
    int* out_length,
    int* out_latency
);

/* aco_algo_manager_cleanup
 * Cleans up any allocated resources for the ACO solver (if needed).
 * Returns 0 (ERR_SUCCESS) on success or negative error code on failure.
 */
int aco_algo_manager_cleanup(AntNetContext* ctx);

#ifdef __cplusplus
}
#endif

#endif /* CPU_ACOV1_ALGO_MANAGER_H */
