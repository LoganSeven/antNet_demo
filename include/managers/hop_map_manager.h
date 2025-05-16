// include/hop_map_manager.h
#ifndef HOP_MAP_MANAGER_H
#define HOP_MAP_MANAGER_H

#include <stddef.h>
#include <stdint.h>
#ifndef _WIN32
#include <pthread.h>
#endif

#include "../types/antnet_network_types.h"  /* NodeData, EdgeData */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * HopMapManager structure with arrays of nodes and edges.
 * This uses NodeData, EdgeData from antnet_network_types.h
 */
typedef struct {
#ifndef _WIN32
    pthread_mutex_t lock;
#endif

    NodeData *start_node;
    NodeData *end_node;
    NodeData *hop_nodes;
    size_t hop_count;

    EdgeData *edges;
    size_t edge_count;
} HopMapManager;

/* Creates a new HopMapManager instance */
HopMapManager* hop_map_manager_create();

/* Destroys and frees the HopMapManager */
void hop_map_manager_destroy(HopMapManager *mgr);

/* Initializes the map with total_nodes (start node + end node + hop_count) */
void hop_map_manager_initialize_map(HopMapManager *mgr, int total_nodes);

/* Creates default edges for up to 3 nearest hops, modifies mgr->edges */
void hop_map_manager_create_default_edges(HopMapManager *mgr);

/*
 * hop_map_manager_export_topology:
 * Exports the topology. The caller provides pointers. The function copies data out.
 * Returns number of nodes copied in *out_node_count, edges in *out_edge_count.
 */
void hop_map_manager_export_topology(HopMapManager *mgr,
                                     NodeData *out_nodes, size_t *out_node_count,
                                     EdgeData *out_edges, size_t *out_edge_count);

#ifdef __cplusplus
}
#endif

#endif /* HOP_MAP_MANAGER_H */
