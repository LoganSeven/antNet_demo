/* Relative Path: include/core/backend_topology.h */
#ifndef BACKEND_TOPOLOGY_H
#define BACKEND_TOPOLOGY_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * This header references NodeData / EdgeData in its function prototype.
 * We define them inline under CFFI_BUILD, otherwise we include the normal header.
 */

#ifdef CFFI_BUILD

struct node_data_s {
    int    node_id;
    int    delay_ms;
    float  x;
    float  y;
    int    radius;
};
typedef struct node_data_s NodeData;

struct edge_data_s {
    int from_id;
    int to_id;
};
typedef struct edge_data_s EdgeData;

#else

#include "../types/antnet_network_types.h"

#endif /* CFFI_BUILD */

/*
 * pub_update_topology: updates the internal graph data within the context.
 * context_id: context handle (index).
 * nodes: array of NodeData, length num_nodes
 * edges: array of EdgeData, length num_edges
 * Returns 0 on success, negative on error.
 */
int pub_update_topology(
    int context_id,
    const NodeData* nodes,
    int num_nodes,
    const EdgeData* edges,
    int num_edges
);

#ifdef __cplusplus
}
#endif

#endif /* BACKEND_TOPOLOGY_H */
