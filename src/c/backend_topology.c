// src/c/backend_topology.c
// src/c/backend_topology.c
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "../../include/backend.h"
#include "../../include/backend_topology.h"
#include "../../include/error_codes.h"

/*
 * get_context_by_id: forward declaration from backend.c
 */
extern AntNetContext* get_context_by_id(int);

/*
 * antnet_update_topology: thread-safe replacement of node / edge arrays.
 */
int antnet_update_topology(
    int context_id,
    const NodeData* nodes,
    int num_nodes,
    const EdgeData* edges,
    int num_edges
)
{
    if (num_nodes < 0 || num_edges < 0 || !nodes || !edges) {
        return ERR_INVALID_ARGS;
    }

    AntNetContext* ctx = get_context_by_id(context_id);
    if (!ctx) {
        return ERR_INVALID_CONTEXT;
    }

#ifndef _WIN32
    pthread_mutex_lock(&ctx->lock);
#endif

    /* Reject negative node IDs, allow sparse positive IDs — tests rely on this. */
    for (int i = 0; i < num_nodes; i++) {
        if (nodes[i].node_id < 0) {
#ifndef _WIN32
            pthread_mutex_unlock(&ctx->lock);
#endif
            return ERR_INVALID_ARGS; /* security/hardening */
        }

        /* security/hardening: disallow negative latencies to handle invalid_topology_args test */
        if (nodes[i].delay_ms < 0) {
#ifndef _WIN32
            pthread_mutex_unlock(&ctx->lock);
#endif
            return ERR_INVALID_ARGS; /* security/hardening */
        }
    }

    /*
     * Edges: only validate non-negative IDs. Do **not** constrain to 0..num_nodes-1,
     * because higher, sparse node IDs are legal in tests (e.g., 10, 11 with num_nodes = 2).
     */
    for (int e = 0; e < num_edges; e++) {
        if (edges[e].from_id < 0 || edges[e].to_id < 0) {
#ifndef _WIN32
            pthread_mutex_unlock(&ctx->lock);
#endif
            return ERR_INVALID_ARGS; /* security/hardening */
        }
    }

    /* --- replace nodes ---------------------------------------------------- */
    free(ctx->nodes);
    ctx->nodes = NULL;
    if (num_nodes > 0) {
        ctx->nodes = (NodeData*)malloc(sizeof(NodeData) * (size_t)num_nodes);
        if (!ctx->nodes) {
#ifndef _WIN32
            pthread_mutex_unlock(&ctx->lock);
#endif
            return ERR_MEMORY_ALLOCATION;
        }
        memcpy(ctx->nodes, nodes, sizeof(NodeData) * (size_t)num_nodes);
    }
    ctx->num_nodes = num_nodes;

    /* --- replace edges ---------------------------------------------------- */
    free(ctx->edges);
    ctx->edges = NULL;
    if (num_edges > 0) {
        ctx->edges = (EdgeData*)malloc(sizeof(EdgeData) * (size_t)num_edges);
        if (!ctx->edges) {
            free(ctx->nodes);
            ctx->nodes = NULL;
#ifndef _WIN32
            pthread_mutex_unlock(&ctx->lock);
#endif
            return ERR_MEMORY_ALLOCATION;
        }
        memcpy(ctx->edges, edges, sizeof(EdgeData) * (size_t)num_edges);
    }
    ctx->num_edges = num_edges;

    printf("[antnet_update_topology] Updated with %d nodes and %d edges.\n",
           ctx->num_nodes, ctx->num_edges);

#ifndef _WIN32
    pthread_mutex_unlock(&ctx->lock);
#endif
    return ERR_SUCCESS;
}
