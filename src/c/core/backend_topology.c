/* Relative Path: src/c/core/backend_topology.c */
/*
 * Updates and validates the network topology (nodes, edges) in a thread-safe manner.
 * Resets algorithm states when the topology changes, preventing stale data.
 * Central place for re-initializing internal memory for ACO or brute-force searches.
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "../../../include/rendering/heatmap_renderer_api.h"
#include "../../../include/core/backend_topology.h"
#include "../../../include/consts/error_codes.h"
#include "../../../include/algo/cpu/cpu_brute_force.h"

extern AntNetContext* priv_get_context_by_id(int);

/*
 * pub_update_topology
 * Updates the internal graph data within the context.
 * The function performs validation on the input data, replaces the nodes and edges
 * in memory, resets the brute-force state, and clears ACO-related data if needed.
 * Thread-safe through context locking.
 */
int pub_update_topology(
    int context_id,
    const NodeData* nodes,
    int num_nodes,
    const EdgeData* edges,
    int num_edges
)
{
    if (num_nodes < 0 || num_edges < 0 || !nodes || !edges) {
        printf("[ERROR] pub_update_topology: Invalid arguments.\n");
        return ERR_INVALID_ARGS;
    }

    AntNetContext* ctx = priv_get_context_by_id(context_id);
    if (!ctx) {
        printf("[ERROR] pub_update_topology: Invalid context ID.\n");
        return ERR_INVALID_CONTEXT;
    }

#ifndef _WIN32
    pthread_mutex_lock(&ctx->lock);
#endif

    for (int i = 0; i < num_nodes; i++) {
        if (nodes[i].node_id < 0) {
            printf("[ERROR] pub_update_topology: Negative node_id found: %d\n", nodes[i].node_id);
#ifndef _WIN32
            pthread_mutex_unlock(&ctx->lock);
#endif
            return ERR_INVALID_ARGS;
        }

        if (nodes[i].delay_ms < 0) {
            printf("[ERROR] pub_update_topology: Negative latency found for node_id %d\n", nodes[i].node_id);
#ifndef _WIN32
            pthread_mutex_unlock(&ctx->lock);
#endif
            return ERR_INVALID_ARGS;
        }
    }

    for (int e = 0; e < num_edges; e++) {
        if (edges[e].from_id < 0 || edges[e].to_id < 0) {
            printf("[ERROR] pub_update_topology: Negative edge IDs found.\n");
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
            printf("[ERROR] pub_update_topology: Node memory allocation failed.\n");
#ifndef _WIN32
            pthread_mutex_unlock(&ctx->lock);
#endif
            return ERR_MEMORY_ALLOCATION;
        }
        memcpy(ctx->nodes, nodes, sizeof(NodeData) * (size_t)num_nodes);
    }
    ctx->num_nodes = num_nodes;

    free(ctx->edges);
    ctx->edges = NULL;
    if (num_edges > 0) {
        ctx->edges = (EdgeData*)malloc(sizeof(EdgeData) * (size_t)num_edges);
        if (!ctx->edges) {
            printf("[ERROR] pub_update_topology: Edge memory allocation failed.\n");
            free(ctx->nodes);
            ctx->nodes = NULL;
#ifndef _WIN32
            pthread_mutex_unlock(&ctx->lock);
#endif
            return ERR_MEMORY_ALLOCATION;
        }
        memcpy(ctx->edges, edges, sizeof(EdgeData) * (size_t)num_edges);
        printf("Edges updated (%d total):\n", num_edges);
    }
    ctx->num_edges = num_edges;

    printf("[pub_update_topology] Updated with %d nodes and %d edges.\n",
           ctx->num_nodes, ctx->num_edges);

    /* Force re-init of Brute Force so it picks up new node counts */
    brute_force_reset_state(ctx);

    /*
     * Also force re-init of ACO memory so next iteration calls aco_v1_init again.
     * This prevents stale pointers or size mismatches on adjacency/pheromones.
     */
    if (ctx->aco_v1.is_initialized) {
        if (ctx->aco_v1.adjacency) {
            free(ctx->aco_v1.adjacency);
            ctx->aco_v1.adjacency = NULL;
        }
        if (ctx->aco_v1.pheromones) {
            free(ctx->aco_v1.pheromones);
            ctx->aco_v1.pheromones = NULL;
        }
        ctx->aco_v1.is_initialized = 0;
    }

#ifndef _WIN32
    pthread_mutex_unlock(&ctx->lock);
#endif

    return ERR_SUCCESS;
}
