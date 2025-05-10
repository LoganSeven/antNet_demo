// include/backend.h
#ifndef BACKEND_H
#define BACKEND_H

/*
 * backend.h
 * Main public API for the AntNet backend.
 *
 * All low-level data structures are defined in dedicated headers.
 * NodeData and EdgeData now come exclusively from antnet_network_types.h,
 * avoiding any duplicate struct declarations when CFFI_BUILD is defined.
 */

#include "antnet_config_types.h"      /* AppConfig                       */
#include "antnet_brute_force_types.h" /* BruteForceState                 */
#include "antnet_path_types.h"        /* AntNetPathInfo                  */
#include "antnet_network_types.h"     /* NodeData, EdgeData              */
#include "error_codes.h"              /* ERR_ constants                  */
#include "backend_thread_defs.h"      /* pthread_mutex_t or placeholder  */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * AntNetContext: stores the ACA context, including nodes, edges,
 * and algorithm-specific parameters.  The structure is visible to CFFI.
 */
typedef struct AntNetContext
{
    /* basic topology parameters */
    int node_count;
    int min_hops;
    int max_hops;

    /* dynamic topology */
    NodeData *nodes;
    int       num_nodes;
    EdgeData *edges;
    int       num_edges;
    int       iteration;

    /* thread safety */
    pthread_mutex_t lock;

    /* random solver best path */
    int  random_best_nodes[1024];
    int  random_best_length;
    int  random_best_latency;

    /* configuration currently loaded */
    AppConfig config;

    /* brute force solver best path */
    int  brute_best_nodes[1024];
    int  brute_best_length;
    int  brute_best_latency;

    /* internal brute force state */
    BruteForceState brute_state;

} AntNetContext;

/* public API */

int antnet_initialize(int node_count, int min_hops, int max_hops);
int antnet_run_iteration(int context_id);
int antnet_shutdown(int context_id);

int antnet_get_best_path(
    int  context_id,
    int *out_nodes,
    int  max_size,
    int *out_path_len,
    int *out_total_latency
);

int antnet_run_all_solvers(
    int  context_id,
    /* aco */
    int *out_nodes_aco,
    int  max_size_aco,
    int *out_len_aco,
    int *out_latency_aco,
    /* random */
    int *out_nodes_random,
    int  max_size_random,
    int *out_len_random,
    int *out_latency_random,
    /* brute */
    int *out_nodes_brute,
    int  max_size_brute,
    int *out_len_brute,
    int *out_latency_brute
);

int antnet_init_from_config(const char *config_path);

#ifdef __cplusplus
}
#endif

#endif /* BACKEND_H */
