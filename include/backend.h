// include/backend.h
// include/backend.h
#ifndef BACKEND_H
#define BACKEND_H

#include "antnet_network_types.h"  /* for NodeData, EdgeData (if needed) */
#include "antnet_path_types.h"     /* for AntNetPathInfo */
#include "antnet_config_types.h"   /* for AppConfig */
#include "antnet_brute_force_types.h"  /* new: for BruteForceState */
#include "error_codes.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * If building on Windows, switch to <windows.h> CRITICAL_SECTION or a similar mechanism.
 * For POSIX, include <pthread.h>.
 */
#ifndef _WIN32
#include <pthread.h>
#endif

/*
 * AntNetContext: stores the ACA context, including nodes, edges, and algorithm parameters.
 */
typedef struct AntNetContext
{
    int node_count;
    int min_hops;
    int max_hops;

    /* Pointers to dynamic topology data */
    NodeData* nodes;
    int       num_nodes;
    EdgeData* edges;
    int       num_edges;
    int       iteration;

    /* Mutex for thread safety */
#ifndef _WIN32
    pthread_mutex_t lock;
#endif

    /*
     * random solver best path data
     * The random solver updates this path if a better solution is found
     */
    int random_best_nodes[1024];
    int random_best_length;
    int random_best_latency;

    /*
     * The loaded configuration is stored here for future usage (min_hops, etc.).
     * We only actively use min_hops, max_hops in the code for now.
     */
    AppConfig config;

    /*
     * brute solver best path data
     * The brute force solver enumerates possible paths and updates this if a better solution is found
     */
    int brute_best_nodes[1024];
    int brute_best_length;
    int brute_best_latency;

    /*
     * Internal iteration state for brute force solver
     */
    BruteForceState brute_state;

} AntNetContext;

/*
 * antnet_initialize: creates a new AntNetContext, returns its context_id (>= 0 on success).
 * Returns ERR_NO_FREE_SLOT if no free slot is available.
 */
int antnet_initialize(int node_count, int min_hops, int max_hops);

/*
 * antnet_run_iteration: executes one iteration of the ACO algorithm for context_id.
 * Returns 0 on success, negative on error.
 */
int antnet_run_iteration(int context_id);

/*
 * antnet_shutdown: frees resources used by context_id.
 * Returns 0 on success, negative on error.
 */
int antnet_shutdown(int context_id);

/*
 * antnet_get_best_path: retrieves the best path found so far for context_id.
 * out_nodes: array to store node indices in path
 * max_size: capacity of out_nodes
 * out_path_len: actual path length
 * out_total_latency: sum of delays along the path
 * Returns 0 on success, negative on error.
 */
int antnet_get_best_path(
    int context_id,
    int* out_nodes,
    int max_size,
    int* out_path_len,
    int* out_total_latency
);

/* antnet_update_topology: declared in backend_topology.h */

/*
 * antnet_run_all_solvers: runs the aco, random, and brute force solvers in sequence,
 * storing each path in separate arrays.
 * out_nodes_aco:    array to store the aco solver path
 * max_size_aco:     capacity of out_nodes_aco
 * out_len_aco:      actual path length of aco solver
 * out_latency_aco:  sum of delays along aco path
 *
 * out_nodes_random: array to store the random solver path
 * max_size_random:  capacity of out_nodes_random
 * out_len_random:   actual path length
 * out_latency_random: sum of delays along random path
 *
 * out_nodes_brute:  array to store the brute solver path
 * max_size_brute:   capacity of out_nodes_brute
 * out_len_brute:    actual path length
 * out_latency_brute: sum of delays along brute path
 *
 * Returns 0 on success, negative on error.
 */
int antnet_run_all_solvers(
    int context_id,
    /* aco */
    int* out_nodes_aco,
    int max_size_aco,
    int* out_len_aco,
    int* out_latency_aco,

    /* random */
    int* out_nodes_random,
    int max_size_random,
    int* out_len_random,
    int* out_latency_random,

    /* brute */
    int* out_nodes_brute,
    int max_size_brute,
    int* out_len_brute,
    int* out_latency_brute
);

/*
 * antnet_init_from_config: loads settings.ini from disk, creates a new context,
 * and sets min_hops, max_hops, node_count, etc. from the loaded config.
 * Returns context_id >= 0 on success, negative on error.
 */
int antnet_init_from_config(const char* config_path);

#ifdef __cplusplus
}
#endif

#endif /* BACKEND_H */
