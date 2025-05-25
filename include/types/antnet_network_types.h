/* Relative Path: include/types/antnet_network_types.h */
/*
 * Defines basic node and edge structures (NodeData, EdgeData), used throughout AntNet.
 * Offers a consistent representation of graph components across different modules and build environments.
*/

#ifndef ANTNET_NETWORK_TYPES_H
#define ANTNET_NETWORK_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CFFI_BUILD

typedef struct node_data_s {
    int    node_id;
    int    delay_ms;
    float  x;
    float  y;
    int    radius;
} NodeData;

typedef struct edge_data_s {
    int from_id;
    int to_id;
} EdgeData;

#else /* CFFI_BUILD */

/* Minimal definitions for cffi. */
typedef struct node_data_s {
    int    node_id;
    int    delay_ms;
    float  x;
    float  y;
    int    radius;
} NodeData;

typedef struct edge_data_s {
    int from_id;
    int to_id;
} EdgeData;

#endif /* CFFI_BUILD */

#ifdef __cplusplus
}
#endif

#endif /* ANTNET_NETWORK_TYPES_H */
