/* Relative Path: include/algo/cpu/cpu_ACOv1_threaded.h */
/*
 * Declares a multi-threaded ACO iteration manager, spinning up one thread per ant.
 * Each ant computes its path and local delta, merging results after completion.
 * Enhances performance by reducing lock contention.
*/


#ifndef CPU_ACOV1_THREADED_H
#define CPU_ACOV1_THREADED_H

#include "../../core/backend.h"

/*
 * aco_v1_run_iteration_threaded
 * Spawns one thread per ant (ctx->aco_v1.num_ants), each computing its own path 
 * and local pheromone deltas. Merges everything into ctx->aco_v1.pheromones at the end.
 * Returns 0 on success, negative on error.
 */
int aco_v1_run_iteration_threaded(AntNetContext *ctx);

#endif /* CPU_ACOV1_THREADED_H */
