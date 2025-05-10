// include/cpu_brute_force.h
#ifndef CPU_BRUTE_FORCE_H
#define CPU_BRUTE_FORCE_H

#include "backend.h"  /* for AntNetContext */

/*
 * brute_force_search_step: enumerates exactly one new path in ascending order
 * of path length L in [ctx->min_hops..ctx->max_hops], considering permutations
 * of candidate internal nodes. If a path is found, it updates ctx->brute_best_*
 * if better. If no path remains, the solver is considered done.
 *
 * The function then copies the best path so far (if any) into:
 * out_nodes, out_path_len, out_total_latency.
 *
 * Returns 0 on success, negative on error.
 */
int brute_force_search_step(
    AntNetContext* ctx,
    int start_id,
    int end_id,
    int* out_nodes,
    int max_size,
    int* out_path_len,
    int* out_total_latency
);

/*
 * brute_force_reset_state: restarts the enumeration from scratch (min_hops),
 * but preserves ctx->brute_best_length and ctx->brute_best_latency.
 * This is used if the topology (nodes/latencies) changes.
 */
void brute_force_reset_state(AntNetContext* ctx);

#endif /* CPU_BRUTE_FORCE_H */
