//src/c/backend.c
#include "backend.h"
#include "backend_topology.h"
#include "error_codes.h"
#include "cpu_ACOv1.h"
#include "random_algo.h"
#include "cpu_brute_force.h"
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
 * Returns ERR_NO_FREE_SLOT if no free slots are available.
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

            // Initialize random solver best path data
            ctx->random_best_length = 0;
            ctx->random_best_latency = 0;
            memset(ctx->random_best_nodes, 0, sizeof(ctx->random_best_nodes));

#ifndef _WIN32
            // Initialize mutex for this context
            pthread_mutex_init(&ctx->lock, NULL);
#endif

            return i; // The newly allocated context ID (>= 0)
        }
    }
    // No free slot found
    return ERR_NO_FREE_SLOT;
}

/*
 * antnet_run_iteration: increments the iteration counter (mock).
 * Thread-safe due to locking. In a real system, this would run one step of the ACO.
 * Returns ERR_INVALID_CONTEXT if the context_id is invalid.
 */
int antnet_run_iteration(int context_id)
{
    AntNetContext* ctx = get_context_by_id(context_id);
    if (!ctx) {
        return ERR_INVALID_CONTEXT;
    }

#ifndef _WIN32
    pthread_mutex_lock(&ctx->lock);
#endif

    // Mock: just increment iteration
    ctx->iteration++;

#ifndef _WIN32
    pthread_mutex_unlock(&ctx->lock);
#endif

    return ERR_SUCCESS; // success
}

/*
 * antnet_get_best_path: retrieves the best path in a mock manner.
 * - out_nodes: array to store node indices
 * - max_size: capacity of out_nodes
 * - out_path_len: actual path length
 * - out_total_latency: sum of delays
 * Returns ERR_SUCCESS on success, negative if error or capacity too small.
 *
 * The original mock data is retained for demonstration, but realistically
 * a real solver or random solver would store best path data in the context.
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
        return ERR_INVALID_CONTEXT;
    }

#ifndef _WIN32
    pthread_mutex_lock(&ctx->lock);
#endif

    // This is still mock data for demonstration if no random solver path is present.
    // The random solver maintains its own best path in ctx->random_best_nodes.
    // If random_best_length > 0, prefer returning that to reflect a real path.
    if (ctx->random_best_length > 0) {
        if (ctx->random_best_length > max_size) {
#ifndef _WIN32
            pthread_mutex_unlock(&ctx->lock);
#endif
            return ERR_ARRAY_TOO_SMALL;
        }
        for (int i = 0; i < ctx->random_best_length; i++) {
            out_nodes[i] = ctx->random_best_nodes[i];
        }
        *out_path_len = ctx->random_best_length;
        *out_total_latency = ctx->random_best_latency;
    } else {
        // Fallback to the older mock approach
        static int mock_nodes[] = {1, 2, 3, 5, 7, 9};
        int length = (int)(sizeof(mock_nodes) / sizeof(mock_nodes[0]));

        if (length > max_size) {
#ifndef _WIN32
            pthread_mutex_unlock(&ctx->lock);
#endif
            return ERR_ARRAY_TOO_SMALL; // user-provided out_nodes array is too small
        }

        for (int i = 0; i < length; i++) {
            out_nodes[i] = mock_nodes[i];
        }
        *out_path_len = length;
        *out_total_latency = 42 + ctx->iteration;
    }

#ifndef _WIN32
    pthread_mutex_unlock(&ctx->lock);
#endif

    return ERR_SUCCESS;
}

/*
 * antnet_shutdown: frees resources for context_id, prints iteration count.
 * Returns ERR_INVALID_CONTEXT if context_id is invalid, ERR_SUCCESS otherwise.
 */
int antnet_shutdown(int context_id)
{
    AntNetContext* ctx = get_context_by_id(context_id);
    if (!ctx) {
        return ERR_INVALID_CONTEXT;
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

    g_context_in_use[context_id] = 0;

    return ERR_SUCCESS;
}

/*
 * antnet_run_all_solvers: runs the aco, random, and brute force solvers in sequence, storing each result
 * in caller-provided buffers. This version calls only the random solver at runtime,
 * while the other calls are commented out for future usage.
 * Returns 0 on success, negative on error.
 */
int antnet_run_all_solvers(
    int context_id,
    // aco
    int* out_nodes_aco,
    int max_size_aco,
    int* out_len_aco,
    int* out_latency_aco,

    // random
    int* out_nodes_random,
    int max_size_random,
    int* out_len_random,
    int* out_latency_random,

    // brute
    int* out_nodes_brute,
    int max_size_brute,
    int* out_len_brute,
    int* out_latency_brute
)
{
    AntNetContext* ctx = get_context_by_id(context_id);
    if (!ctx) {
        return ERR_INVALID_CONTEXT;
    }

#ifndef _WIN32
    pthread_mutex_lock(&ctx->lock);
#endif

    // For demonstration, skip the actual ACO call and store empty results:
    // int rc = aco_v1_search_path(
    //     ctx, 0, 1, out_nodes_aco, max_size_aco, out_len_aco, out_latency_aco
    // );
    // if (rc != ERR_SUCCESS) {
    // #ifndef _WIN32
    //     pthread_mutex_unlock(&ctx->lock);
    // #endif
    //     return rc;
    // }
    *out_len_aco = 0;
    *out_latency_aco = 0;

#ifndef _WIN32
    pthread_mutex_unlock(&ctx->lock);
#endif

    // Call the random solver once; it attempts a new random path and returns the best so far
    int rc = random_search_path(
        ctx, 0, 1,
        out_nodes_random, max_size_random,
        out_len_random, out_latency_random
    );
    if (rc != ERR_SUCCESS) {
        return rc;
    }

#ifndef _WIN32
    pthread_mutex_lock(&ctx->lock);
#endif

    // For demonstration, skip the brute force solver and store empty results:
    // rc = brute_force_path(
    //     ctx, 0, 1, out_nodes_brute, max_size_brute, out_len_brute, out_latency_brute
    // );
    // if (rc != ERR_SUCCESS) {
    // #ifndef _WIN32
    //     pthread_mutex_unlock(&ctx->lock);
    // #endif
    //     return rc;
    // }
    *out_len_brute = 0;
    *out_latency_brute = 0;

#ifndef _WIN32
    pthread_mutex_unlock(&ctx->lock);
#endif

    return ERR_SUCCESS;
}
