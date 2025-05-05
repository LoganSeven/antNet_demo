#ifndef BACKEND_TOPOLOGY_H
#define BACKEND_TOPOLOGY_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * NodeData: holds node ID and node-specific delay (in milliseconds).
 */
typedef struct NodeData {
    int node_id;
    int delay_ms;
} NodeData;

/**
 * EdgeData: holds directed connection from one node to another.
 */
typedef struct EdgeData {
    int from_id;
    int to_id;
} EdgeData;

/**
 * antnet_update_topology: updates the internal graph data within the context.
 * context_id: context handle (index).
 * nodes: array of NodeData, with length num_nodes.
 * edges: array of EdgeData, with length num_edges.
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

#endif // BACKEND_TOPOLOGY_H
