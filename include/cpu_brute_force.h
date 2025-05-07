//include/cpu_brute_force.h
#ifndef CPU_BRUTE_FORCE_H
#define CPU_BRUTE_FORCE_H

/*
 * cpu_brute_force.h
 * Provides a brute force algorithm to find the path with the lowest total latency
 * from start_id to end_id, constrained by min_hops and max_hops.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "backend.h"

/*
 * brute_force_path: enumerates all possible paths (within min_hops..max_hops)
 * from start_id to end_id, selects the minimum-latency path, and copies it
 * to out_nodes. Sets *out_path_len and *out_total_latency. Returns 0 on success,
 * negative if no path is found or an error occurs.
 */
int brute_force_path(
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

#endif // CPU_BRUTE_FORCE_H