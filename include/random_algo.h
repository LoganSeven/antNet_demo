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
 * random_search_path: attempts to find a random path from start_id to end_id,
 * constrained by ctx->min_hops and ctx->max_hops. On success, places the path
 * into out_nodes, sets *out_path_len, and *out_total_latency. Returns 0 on success,
 * negative if no path can be found or an error occurs.
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

#endif // RANDOM_ALGO_H
