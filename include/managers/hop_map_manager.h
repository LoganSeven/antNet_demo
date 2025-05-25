/* Relative Path: include/managers/hop_map_manager.h */
/*
 * Maintains a manager for hop-based node placement with optional random delays.
 * Creates and updates node positions, edges, and exports the resulting topology.
 * Facilitates dynamic graph generation for demonstrations or simulation scenarios.
*/

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

    /* new fields for adjustable node delay range */
    int default_min_delay;
    int default_max_delay;

} HopMapManager;

/* Creates a new HopMapManager instance */
HopMapManager* hop_map_manager_create();

/* Destroys and frees the HopMapManager */
void hop_map_manager_destroy(HopMapManager *mgr);

/* Allows setting the random delay range for node latencies. */
void hop_map_manager_set_delay_range(HopMapManager *mgr, int min_delay, int max_delay);

/* Initializes the map with total_nodes (start node + end node + hop_count).
 * If total_nodes is unchanged, it skips reallocation. */
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

/*
 * hop_map_manager_recalc_positions
 * Recomputes x,y positions of existing nodes (start, end, and hops),
 * based on the new scene width and height. Does not alter latencies or node arrays.
 * This allows dynamic resizing of the scene without re-creating nodes.
 * The function ensures horizontal spacing and vertical centering.
 */
void hop_map_manager_recalc_positions(HopMapManager *mgr,
                                      float scene_width,
                                      float scene_height);

#ifdef __cplusplus
}
#endif

#endif /* HOP_MAP_MANAGER_H */
