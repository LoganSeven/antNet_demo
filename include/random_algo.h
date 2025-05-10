// include/backend_topology.h
#ifndef BACKEND_TOPOLOGY_H
#define BACKEND_TOPOLOGY_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * This header references NodeData / EdgeData in its function prototype.
 * The canonical definitions of these structs live in antnet_network_types.h.
 * This file no longer redeclares them under CFFI_BUILD, to avoid duplication.
 */

#include "antnet_network_types.h"

/*
 * antnet_update_topology: updates the internal graph data within the context.
 * context_id: context handle (index).
 * nodes: array of NodeData, length num_nodes
 * edges: array of EdgeData, length num_edges
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
