// include/antnet_path_types.h
#ifndef ANTNET_PATH_TYPES_H
#define ANTNET_PATH_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * AntNetPathInfo: not used heavily in this demo, just an example struct to store path data.
 * Moved here from backend.h to reduce rebuild dependencies.
 */
typedef struct {
    int* nodes;
    int  node_count;
    int  total_latency;
} AntNetPathInfo;

#ifdef __cplusplus
}
#endif

#endif /* ANTNET_PATH_TYPES_H */
