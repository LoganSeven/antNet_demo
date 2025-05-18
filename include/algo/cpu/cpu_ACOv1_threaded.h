/* Relative Path: include/algo/cpu/cpu_ACOv1_threaded.h */

/*
 * cpu_ACOv1_threaded.h
 * Declares a multi-threaded runner for the ACO iteration. Each ant executes in parallel,
 * storing local deltas, then merging them at the end.
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
