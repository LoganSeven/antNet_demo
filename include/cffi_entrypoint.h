/* Relative Path: include/cffi_entrypoint.h */
#ifndef CFFI_ENTRYPOINT_H
#define CFFI_ENTRYPOINT_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * cffi_entrypoint.h
 *
 * A single aggregator header for the cffi pipeline.
 * Ensures NodeData / EdgeData are defined before references like "NodeData *nodes;"
 * in AntNetContext or antnet_update_topology,
 * and ensures that antnet_initialize(), antnet_run_iteration(), etc.
 * are actually seen by pycparser so cffi exports them to Python.
 *
 * For normal C builds, this file is not used. This is only for cffi parsing.
 */

/* 1) The raw definitions of NodeData / EdgeData, etc. */
#include "./types/antnet_network_types.h"

/* 2) Additional types that might appear in function signatures */
#include "./types/antnet_config_types.h"
#include "./types/antnet_brute_force_types.h"
#include "./types/antnet_path_types.h"
#include "./core/backend_thread_defs.h"
#include "./consts/error_codes.h"

/* 3) The main backend headers that declare the functions Python needs */
#include "./core/backend.h"            // antnet_initialize, antnet_run_iteration, etc.
#include "./core/backend_topology.h"   // antnet_update_topology
#include "./core/score_evaluation.h"
/* 4) Other solver modules or managers that Python calls or references */
#include "./algo/cpu/cpu_random_algo.h"        // random_search_path
#include "./algo/cpu/cpu_brute_force.h"  // brute_force_search_step
#include "./algo/cpu/cpu_ACOv1.h"          // aco_v1_...
#include "./managers/config_manager.h"     // config_load, config_save, ...
#include "./managers/hop_map_manager.h"

/* 5) generators , e.g. heatmap_renderer*/
#include "./rendering/heatmap_renderer.h"
#include "./rendering/heatmap_renderer_async.h"

#ifdef __cplusplus
}
#endif

#endif /* CFFI_ENTRYPOINT_H */
