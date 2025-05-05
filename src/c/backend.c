#include "backend.h"
#include "backend_topology.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Maximum number of parallel contexts
#define MAX_CONTEXTS 16

// Storage for all contexts
static AntNetContext g_contexts[MAX_CONTEXTS];
static int g_context_in_use[MAX_CONTEXTS] = {0};  // 0 = free, 1 = in use

/*
 * Helper function: retrieve pointer to a valid context by ID,
 * or return NULL if the ID is out of range or not in use.
 */
AntNetContext* get_context_by_id(int context_id)
{
    if (context_id < 0 || context_id >= MAX_CONTEXTS) {
        return NULL;
    }
    if (!g_context_in_use[context_id]) {
        return NULL;
    }
    return &g_contexts[context_id];
}

/*
 * antnet_initialize: creates a new AntNetContext in a free slot, returns context_id.
 * Returns -1 if no free slots are available.
 */
int antnet_initialize(int node_count, int min_hops, int max_hops)
{
    for (int i = 0; i < MAX_CONTEXTS; i++) {
        if (!g_context_in_use[i]) {
            g_context_in_use[i] = 1;

            // Initialize the struct
            AntNetContext* ctx = &g_contexts[i];
            ctx->node_count = node_count;
            ctx->min_hops   = min_hops;
            ctx->max_hops   = max_hops;
            ctx->iteration  = 0;
            ctx->nodes      = NULL;
            ctx->num_nodes  = 0;
            ctx->edges      = NULL;
            ctx->num_edges  = 0;

#ifndef _WIN32
            // Initialize mutex for this context
            pthread_mutex_init(&ctx->lock, NULL);
#endif

            // Optional debug
            // printf("[antnet_initialize] Created context %d\n", i);

            return i; // the newly allocated context ID
        }
    }
    return -1; // no free slot
}

/*
 * antnet_run_iteration: increments the iteration counter (mock).
 * Thread-safe due to locking. In a real system, this would run one step of the ACO.
 */
int antnet_run_iteration(int context_id)
{
    AntNetContext* ctx = get_context_by_id(context_id);
    if (!ctx) {
        return -1; // invalid context
    }

#ifndef _WIN32
    pthread_mutex_lock(&ctx->lock);
#endif

    // Mock: just increment iteration
    ctx->iteration++;

#ifndef _WIN32
    pthread_mutex_unlock(&ctx->lock);
#endif

    return 0; // success
}

/*
 * antnet_get_best_path: retrieves the best path in a mock manner.
 * - out_nodes: array to store node indices
 * - max_size: capacity of out_nodes
 * - out_path_len: actual path length
 * - out_total_latency: sum of delays
 * Returns 0 on success, negative if error or capacity too small.
 */
int antnet_get_best_path(
    int context_id,
    int* out_nodes,
    int max_size,
    int* out_path_len,
    int* out_total_latency
)
{
    AntNetContext* ctx = get_context_by_id(context_id);
    if (!ctx) {
        return -1; // invalid context
    }

#ifndef _WIN32
    pthread_mutex_lock(&ctx->lock);
#endif

    // This is still mock data for demonstration. Implementation of a real path search
    // would iterate over ctx->nodes, ctx->edges, etc.
    static int mock_nodes[] = {1, 2, 3, 5, 7, 9};
    int length = (int)(sizeof(mock_nodes) / sizeof(mock_nodes[0]));

    if (length > max_size) {
#ifndef _WIN32
        pthread_mutex_unlock(&ctx->lock);
#endif
        return -2; // user-provided out_nodes array is too small
    }

    // Copy mock path to out_nodes
    for (int i = 0; i < length; i++) {
        out_nodes[i] = mock_nodes[i];
    }

    // Return the path length and a mock total latency (depends on iteration)
    *out_path_len = length;
    *out_total_latency = 42 + ctx->iteration;

#ifndef _WIN32
    pthread_mutex_unlock(&ctx->lock);
#endif

    return 0;
}

/*
 * antnet_shutdown: frees resources for context_id, prints iteration count.
 */
int antnet_shutdown(int context_id)
{
    AntNetContext* ctx = get_context_by_id(context_id);
    if (!ctx) {
        return -1; // invalid context
    }

#ifndef _WIN32
    pthread_mutex_lock(&ctx->lock);
#endif

    // Free allocated arrays
    if (ctx->nodes) {
        free(ctx->nodes);
        ctx->nodes = NULL;
    }
    if (ctx->edges) {
        free(ctx->edges);
        ctx->edges = NULL;
    }

    printf("[antnet_shutdown] context %d final iteration: %d\n",
           context_id, ctx->iteration);

#ifndef _WIN32
    pthread_mutex_unlock(&ctx->lock);
    pthread_mutex_destroy(&ctx->lock);
#endif

    // Mark the slot free
    g_context_in_use[context_id] = 0;

    return 0;
}
