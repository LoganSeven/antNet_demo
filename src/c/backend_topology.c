// src/c/backend_topology.c
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "../../include/backend.h"
#include "../../include/backend_topology.h"
#include "../../include/error_codes.h"
#include "../../include/cpu_brute_force.h"

extern AntNetContext* get_context_by_id(int);

int antnet_update_topology(
    int context_id,
    const NodeData* nodes,
    int num_nodes,
    const EdgeData* edges,
    int num_edges
)
{
    if (num_nodes < 0 || num_edges < 0 || !nodes || !edges) {
        printf("[ERROR] antnet_update_topology: Invalid arguments.\n");
        return ERR_INVALID_ARGS;
    }

    AntNetContext* ctx = get_context_by_id(context_id);
    if (!ctx) {
        printf("[ERROR] antnet_update_topology: Invalid context ID.\n");
        return ERR_INVALID_CONTEXT;
    }

#ifndef _WIN32
    pthread_mutex_lock(&ctx->lock);
#endif

    for (int i = 0; i < num_nodes; i++) {
        if (nodes[i].node_id < 0) {
            printf("[ERROR] antnet_update_topology: Negative node_id found: %d\n", nodes[i].node_id);
#ifndef _WIN32
            pthread_mutex_unlock(&ctx->lock);
#endif
            return ERR_INVALID_ARGS;
        }

        if (nodes[i].delay_ms < 0) {
            printf("[ERROR] antnet_update_topology: Negative latency found for node_id %d\n", nodes[i].node_id);
#ifndef _WIN32
            pthread_mutex_unlock(&ctx->lock);
#endif
            return ERR_INVALID_ARGS;
        }
    }

    for (int e = 0; e < num_edges; e++) {
        if (edges[e].from_id < 0 || edges[e].to_id < 0) {
            printf("[ERROR] antnet_update_topology: Negative edge IDs found.\n");
#ifndef _WIN32
            pthread_mutex_unlock(&ctx->lock);
#endif
            return ERR_INVALID_ARGS;
        }
    }

    free(ctx->nodes);
    ctx->nodes = NULL;
    if (num_nodes > 0) {
        ctx->nodes = (NodeData*)malloc(sizeof(NodeData) * (size_t)num_nodes);
        if (!ctx->nodes) {
            printf("[ERROR] antnet_update_topology: Node memory allocation failed.\n");
#ifndef _WIN32
            pthread_mutex_unlock(&ctx->lock);
#endif
            return ERR_MEMORY_ALLOCATION;
        }
        memcpy(ctx->nodes, nodes, sizeof(NodeData) * (size_t)num_nodes);

        printf("[DEBUG] Nodes updated (%d total):\n", num_nodes);
    }
    ctx->num_nodes = num_nodes;

    free(ctx->edges);
    ctx->edges = NULL;
    if (num_edges > 0) {
        ctx->edges = (EdgeData*)malloc(sizeof(EdgeData) * (size_t)num_edges);
        if (!ctx->edges) {
            printf("[ERROR] antnet_update_topology: Edge memory allocation failed.\n");
            free(ctx->nodes);
            ctx->nodes = NULL;
#ifndef _WIN32
            pthread_mutex_unlock(&ctx->lock);
#endif
            return ERR_MEMORY_ALLOCATION;
        }
        memcpy(ctx->edges, edges, sizeof(EdgeData) * (size_t)num_edges);

        printf("[DEBUG] Edges updated (%d total):\n", num_edges);
        for (int e = 0; e < num_edges; ++e) {
            printf("  [DEBUG] Edge[%d]: from_id=%d, to_id=%d\n",
                   e, ctx->edges[e].from_id, ctx->edges[e].to_id);
        }
    }
    ctx->num_edges = num_edges;

    printf("[antnet_update_topology] Updated with %d nodes and %d edges.\n",
           ctx->num_nodes, ctx->num_edges);

    brute_force_reset_state(ctx);

#ifndef _WIN32
    pthread_mutex_unlock(&ctx->lock);
#endif

    return ERR_SUCCESS;
}
