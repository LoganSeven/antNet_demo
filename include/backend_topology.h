// include/backend_topology.h
#ifndef BACKEND_TOPOLOGY_H
#define BACKEND_TOPOLOGY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "antnet_network_types.h"  /* NodeData, EdgeData */

/*
 * antnet_update_topology: updates the internal graph data within the context.
 * context_id: context handle (index).
 * nodes: array of NodeData, with length num_nodes.
 * edges: array of EdgeData, with length num_edges.
 * Returns 0 on success, negative on error.
 */
int antnet_update_topology(
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
