/* Relative Path: include/algo/cpu/cpu_random_algo.h */
/*
 * Provides a baseline random pathfinding approach for AntNet.
 * Randomly selects intermediate hops, forms a path, updates the best route if improved.
 * Simple but valuable for performance comparisons against advanced algorithms.
*/

#ifndef RANDOM_ALGO_H
#define RANDOM_ALGO_H

#include "../../core/backend.h"  /* for AntNetContext */
#include "../../consts/error_codes.h"  /* for ERR_SUCCESS and error codes */

/*
 * random_search_path:
 * Attempts to build a random path between start_id and end_id.
 * 
 * The path is built as:
 *   start_id + [randomly shuffled intermediate hops] + end_id
 * The number of intermediate hops is chosen randomly in [min_hops..max_hops],
 * clamped to available node count (excluding start and end).
 *
 * On success, the result is copied into out_nodes[0..*out_path_len-1]
 * and total latency is written to *out_total_latency.
 *
 * Thread Safety:
 *   This function does NOT acquire ctx->lock.
 *   The caller must lock ctx before calling (e.g. from bridging or top-level API).
 *
 * Returns:
 *   ERR_SUCCESS (0)              on success
 *   ERR_INVALID_ARGS (< 0)       if input pointers are null or arguments are invalid
 *   ERR_NO_TOPOLOGY              if ctx->nodes is null or empty
 *   ERR_NO_PATH_FOUND            if no valid intermediate nodes are available
 *   ERR_ARRAY_TOO_SMALL          if out_nodes[] buffer is too small
 *   ERR_MEMORY_ALLOCATION        if allocation fails
 */
int random_search_path(
    AntNetContext* ctx,
    int start_id,
    int end_id,
    int* out_nodes,
    int max_size,
    int* out_path_len,
    int* out_total_latency
);

#endif /* RANDOM_ALGO_H */
