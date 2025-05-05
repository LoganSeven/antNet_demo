#ifndef BACKEND_H
#define BACKEND_H

#include "backend_topology.h"

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

/**
 * AntNetPathInfo: not used heavily in this demo, just an example struct to store path data.
 */
typedef struct {
    int* nodes;
    int  node_count;
    int  total_latency;
} AntNetPathInfo;


/**
 * AntNetContext: stores the ACA context, including nodes, edges, and algorithm parameters.
 */
typedef struct AntNetContext
{
    int node_count;
    int min_hops;
    int max_hops;

    // Pointers to dynamic topology data
    NodeData* nodes;
    int num_nodes;
    EdgeData* edges;
    int num_edges;
    int iteration;

    // Mutex for thread safety
#ifndef _WIN32
    pthread_mutex_t lock;
#endif

    // ... (other fields related to ACA, threads, etc.)
} AntNetContext;

/**
 * antnet_initialize: creates a new AntNetContext, returns its context_id (>= 0 on success).
 */
int antnet_initialize(int node_count, int min_hops, int max_hops);

/**
 * antnet_run_iteration: executes one iteration of the ACO algorithm for context_id.
 * Returns 0 on success, negative on error.
 */
int antnet_run_iteration(int context_id);

/**
 * antnet_shutdown: frees resources used by context_id.
 * Returns 0 on success, negative on error.
 */
int antnet_shutdown(int context_id);

/**
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

/*
 * antnet_update_topology: declared in backend_topology.h
 * This call replaces the existing node/edge data in the context.
 */

#ifdef __cplusplus
}
#endif

#endif // BACKEND_H
