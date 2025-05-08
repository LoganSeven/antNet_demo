//include/antnet_network_types.h
#ifndef ANTNET_NETWORK_TYPES_H
#define ANTNET_NETWORK_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * NodeData: holds node ID and node-specific delay (in milliseconds), as well as
 *           geometry data (x, y, radius) for the hop_map_manager usage.
 *           The geometry fields are optional for the ACA logic but remain
 *           unified here to avoid ABI mismatches or repeated definitions.
 */
typedef struct NodeData {
    int    node_id;
    int    delay_ms;   /* Delay in milliseconds */

    /* geometry fields used by hop_map_manager; can be ignored if unused */
    float  x;
    float  y;
    int    radius;
} NodeData;

/*
 * EdgeData: holds directed connection from one node to another.
 */
typedef struct EdgeData {
    int from_id;
    int to_id;
} EdgeData;

#ifdef __cplusplus
}
#endif

#endif /* ANTNET_NETWORK_TYPES_H */
