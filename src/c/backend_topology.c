//src/c/backend_topology.c
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "backend.h"
#include "backend_topology.h"
#include "error_codes.h"

/**
 * get_context_by_id: forward declaration from backend.c
 * Only used here for retrieving the context pointer.
 */
extern AntNetContext* get_context_by_id(int);

/*
 * antnet_update_topology: replaces the context's node/edge arrays with the given ones.
 * The function is thread-safe due to the context-level mutex.
 */
int antnet_update_topology(
    int context_id,
    const NodeData* nodes,
    int num_nodes,
    const EdgeData* edges,
    int num_edges
)
{
    AntNetContext* ctx = get_context_by_id(context_id);
    if (!ctx) {
        return ERR_INVALID_CONTEXT; // invalid context
    }

#ifndef _WIN32
    pthread_mutex_lock(&ctx->lock);
#endif

    // Free old node array
    if (ctx->nodes) {
        free(ctx->nodes);
        ctx->nodes = NULL;
    }

    // Free old edge array
    if (ctx->edges) {
        free(ctx->edges);
        ctx->edges = NULL;
    }

    // Allocate new node array
    if (num_nodes > 0) {
        ctx->nodes = (NodeData*)malloc(sizeof(NodeData) * num_nodes);
        if (!ctx->nodes) {
#ifndef _WIN32
            pthread_mutex_unlock(&ctx->lock);
#endif
            return ERR_MEMORY_ALLOCATION; // memory allocation failure
        }
        memcpy(ctx->nodes, nodes, sizeof(NodeData) * num_nodes);
    } else {
        ctx->nodes = NULL;
    }
    ctx->num_nodes = num_nodes;

    // Allocate new edge array
    if (num_edges > 0) {
        ctx->edges = (EdgeData*)malloc(sizeof(EdgeData) * num_edges);
        if (!ctx->edges) {
            // Release node array if edge array fails
            free(ctx->nodes);
            ctx->nodes = NULL;
#ifndef _WIN32
            pthread_mutex_unlock(&ctx->lock);
#endif
            return ERR_MEMORY_ALLOCATION;
        }
        memcpy(ctx->edges, edges, sizeof(EdgeData) * num_edges);
    } else {
        ctx->edges = NULL;
    }
    ctx->num_edges = num_edges;

    // If needed, re-initialize any path-finding or pheromone structures here.

    printf("[antnet_update_topology] Updated with %d nodes and %d edges.\n",
           ctx->num_nodes, ctx->num_edges);

#ifndef _WIN32
    pthread_mutex_unlock(&ctx->lock);
#endif

    return ERR_SUCCESS;
}