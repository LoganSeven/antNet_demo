// include/cpu_brute_force.h
#ifndef CPU_BRUTE_FORCE_H
#define CPU_BRUTE_FORCE_H

#include "../../core/backend.h"  /* for AntNetContext */

/*
 * brute_force_search_step: enumerates exactly one new path in ascending order
 * of path length L in [ctx->min_hops..ctx->max_hops], considering permutations
 * and combinations of candidate internal nodes. If a better path is found,
 * it updates ctx->brute_best_*.
 *
 * The best path found so far (if any) is immediately copied into:
 * out_nodes, out_path_len, out_total_latency.
 *
 * Returns 0 on success, ERR_NO_PATH_FOUND if no valid path exists,
 * or a negative error code on other errors.
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
 * resetting permutation and combination states but preserving the best path
 * found so far. Should be called if the topology changes.
 */
void brute_force_reset_state(AntNetContext* ctx);

#endif /* CPU_BRUTE_FORCE_H */