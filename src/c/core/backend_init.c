/* Relative Path: src/c/core/backend_init.c */
/*
 * backend_init.c
 * Implements the global context arrays, locking, initialization, and shutdown routines.
 */

#include "../../../include/core/backend_init.h"
#include "../../../include/consts/error_codes.h"
#include "../../../include/managers/config_manager.h"
#include "../../../include/types/antnet_sasa_types.h"
#include "../../../include/managers/ranking_manager.h"  
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#ifndef _WIN32
#include <pthread.h>
#endif

#define MAX_CONTEXTS 16

/* Global arrays, lock, and usage markers for contexts. */
static AntNetContext g_contexts[MAX_CONTEXTS];
static int g_context_in_use[MAX_CONTEXTS] = {0};

#ifndef _WIN32
static pthread_mutex_t g_contexts_lock = PTHREAD_MUTEX_INITIALIZER;
#endif

/*
 * get_context_by_id
 * Retrieves the pointer to the context if in range and in use, else returns NULL.
 */
AntNetContext* priv_get_context_by_id(int context_id)
{
    if (context_id < 0 || context_id >= MAX_CONTEXTS)
    {
        return NULL;
    }
#ifndef _WIN32
    pthread_mutex_lock(&g_contexts_lock);
#endif
    int in_use = g_context_in_use[context_id];
#ifndef _WIN32
    pthread_mutex_unlock(&g_contexts_lock);
#endif

    return in_use ? &g_contexts[context_id] : NULL;
}

/*
 * antnet_initialize
 * Creates a new context if there is a free slot, initializes default fields,
 * sets up random/bf/aco states, and returns the context_id on success.
 */
int pub_initialize(int node_count, int min_hops, int max_hops)
{
#ifndef _WIN32
    pthread_mutex_lock(&g_contexts_lock);
#endif
    for (int i = 0; i < MAX_CONTEXTS; i++)
    {
        if (!g_context_in_use[i])
        {
            g_context_in_use[i] = 1;
#ifndef _WIN32
            pthread_mutex_unlock(&g_contexts_lock);
#endif
            AntNetContext* ctx = &g_contexts[i];
            ctx->node_count = node_count;
            ctx->min_hops   = min_hops;
            ctx->max_hops   = max_hops;
            ctx->iteration  = 0;
            ctx->nodes      = NULL;
            ctx->edges      = NULL;
            ctx->num_nodes  = 0;
            ctx->num_edges  = 0;

            ctx->random_best_length  = 0;
            ctx->random_best_latency = 0;
            memset(ctx->random_best_nodes, 0, sizeof(ctx->random_best_nodes));

            pub_config_set_defaults(&ctx->config);
            ctx->config.set_nb_nodes = node_count;
            ctx->config.min_hops     = min_hops;
            ctx->config.max_hops     = max_hops;

            ctx->brute_best_length  = 0;
            ctx->brute_best_latency = 0;
            memset(ctx->brute_best_nodes, 0, sizeof(ctx->brute_best_nodes));
            memset(&ctx->brute_state, 0, sizeof(ctx->brute_state));

            ctx->aco_best_length  = 0;
            ctx->aco_best_latency = 0;
            memset(ctx->aco_best_nodes, 0, sizeof(ctx->aco_best_nodes));
            memset(&ctx->aco_v1, 0, sizeof(ctx->aco_v1));

#ifndef _WIN32
            pthread_mutex_init(&ctx->lock, NULL);
#endif

            /* SASA addition: initialize SASA states for ACO, Random, Brute to default */
            priv_init_sasa_state(&ctx->aco_sasa);
            priv_init_sasa_state(&ctx->random_sasa);
            priv_init_sasa_state(&ctx->brute_sasa);

            /* NEW: default SASA coefficients */
            ctx->sasa_coeffs.alpha = 0.4;
            ctx->sasa_coeffs.beta  = 0.4;
            ctx->sasa_coeffs.gamma = 0.2;

            return i;
        }
    }
#ifndef _WIN32
    pthread_mutex_unlock(&g_contexts_lock);
#endif
    return ERR_NO_FREE_SLOT;
}

/*
 * pub_shutdown
 * Frees all allocated memory associated with the context, destroys locks,
 * and marks the slot as unused. Thread-safe with final destruction after unlock.
 */
int pub_shutdown(int context_id)
{
    AntNetContext* ctx = priv_get_context_by_id(context_id);
    if (!ctx)
    {
        return ERR_INVALID_CONTEXT;
    }

#ifndef _WIN32
    pthread_mutex_lock(&ctx->lock);
#endif
    if (ctx->nodes)
    {
        free(ctx->nodes);
        ctx->nodes = NULL;
    }
    if (ctx->edges)
    {
        free(ctx->edges);
        ctx->edges = NULL;
    }
    if (ctx->aco_v1.adjacency)
    {
        free(ctx->aco_v1.adjacency);
        ctx->aco_v1.adjacency = NULL;
    }
    if (ctx->aco_v1.pheromones)
    {
        free(ctx->aco_v1.pheromones);
        ctx->aco_v1.pheromones = NULL;
    }

    printf("[antnet_shutdown] context %d final iteration: %d\n", context_id, ctx->iteration);

#ifndef _WIN32
    pthread_mutex_unlock(&ctx->lock);
    pthread_mutex_destroy(&ctx->lock);
#endif

#ifndef _WIN32
    pthread_mutex_lock(&g_contexts_lock);
#endif
    g_context_in_use[context_id] = 0;
#ifndef _WIN32
    pthread_mutex_unlock(&g_contexts_lock);
#endif
    return ERR_SUCCESS;
}
