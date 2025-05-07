#ifndef HOP_MAP_MANAGER_H
#define HOP_MAP_MANAGER_H

#include <stddef.h>
#include <stdint.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Simple struct holding node data */
typedef struct {
    int node_id;
    float x;
    float y;
    int radius;
    uint32_t delay_ms;
    /* color can be stored as an int or separate struct; here just stored as an int placeholder if needed */
    /* or store a fixed-length char[] for the color code if needed */
} NodeData;

/* Simple struct holding edge data */
typedef struct {
    int from_id;
    int to_id;
} EdgeData;

/* HopMapManager structure with arrays of nodes and edges */
typedef struct {
    pthread_mutex_t lock;

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

/* Exports the topology. The caller provides pointers. The function copies data out.
   Returns number of nodes copied in *out_node_count, edges in *out_edge_count. */
void hop_map_manager_export_topology(HopMapManager *mgr,
                                     NodeData *out_nodes, size_t *out_node_count,
                                     EdgeData *out_edges, size_t *out_edge_count);

#ifdef __cplusplus
}
#endif

#endif
