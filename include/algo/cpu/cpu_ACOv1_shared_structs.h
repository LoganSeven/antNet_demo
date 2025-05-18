/* Relative Path: include/algo/cpu/cpu_ACOv1_shared_structs.h */

/*
 * 
 * Data structures and utility functions to manage per-thread pheromone deltas
 * and merge them safely into the shared ACO context.
 */

#ifndef CPU_ACOv1_SHARED_STRUCTS_H
#define CPU_ACOv1_SHARED_STRUCTS_H

#include "../../core/backend.h"

/*
 * AcoThreadLocalData
 * Holds local pheromone increments and any intermediate best path info for one ant thread.
 */
typedef struct AcoThreadLocalData
{
    float *delta_pheromones; /* local pheromone increments, size = n*n where n=ctx->aco_v1.pheromone_size */
    int    best_path[1024];
    int    best_length;
    int    best_latency;
} AcoThreadLocalData;

/*
 * aco_shared_create_local_data
 * Allocates and initializes AcoThreadLocalData for one ant thread.
 * Returns pointer on success, or NULL on allocation failure.
 */
AcoThreadLocalData *aco_shared_create_local_data(int pheromone_size);

/*
 * aco_shared_free_local_data
 * Frees the memory in AcoThreadLocalData. The pointer can be re-used only after a fresh call
 * to aco_shared_create_local_data.
 */
void aco_shared_free_local_data(AcoThreadLocalData *data);

/*
 * aco_shared_merge_deltas
 * Merges each thread's delta_pheromones into ctx->aco_v1.pheromones under a single lock.
 * Also updates the global best path if a thread found a better one.
 * Returns 0 on success, negative on error.
 */
int aco_shared_merge_deltas(AntNetContext *ctx, AcoThreadLocalData **thread_locals, int count);

#endif /* CPU_ACOv1_SHARED_STRUCTS_H */
