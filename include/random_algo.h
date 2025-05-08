//include/random_algo.h
#ifndef RANDOM_ALGO_H
#define RANDOM_ALGO_H

/*
 * random_algo.h
 * Provides a simple random path search to find a route from start_id to end_id.
 * This approach does not guarantee the best path; it merely returns any found path.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "backend.h"

/*
 * random_search_path: attempts one new random path from start_id to end_id
 * by randomly selecting a count of nodes in [ctx->min_hops..ctx->max_hops] from the
 * available nodes, then updating ctx->random_best_* if the path is better.
 * On success, returns 0 and copies the current best path (which may have been improved).
 * If no path can be found or an error occurs, returns negative.
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

#ifdef __cplusplus
}
#endif

#endif /* RANDOM_ALGO_H */
