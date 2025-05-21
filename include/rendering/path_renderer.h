/* Relative Path: include/rendering/path_renderer.h */
#include "../../include/core/backend.h" /* for get_context_by_id(...) */
#ifndef PATH_RENDERER_H
#define PATH_RENDERER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * pub_render_path_grid
 *
 * Computes a 90-degree segmented path for the given list of node IDs
 * and writes the resulting float (x,y) coordinates into out_coords.
 *
 * Each pair of adjacent node_ids is routed on a discrete grid,
 * avoiding other nodes as obstacles. The final path is returned
 * as a list of consecutive (x,y) points forming a polyline.
 *
 * Thread-safe usage requires context locking by the caller,
 * or by the function that calls it.
 */
int pub_render_path_grid(
    int context_id,
    const int* node_ids,
    int node_count,
    float offset_x,
    float offset_y,
    float* out_coords,
    int max_coords,
    int* out_count
);

/**
 * priv_render_path_grid
 *
 * Internal rendering function used by antnet_render_path_grid(...).
 * The context must already be locked externally. Do not call this
 * function directly unless you have acquired the necessary lock.
 */
int priv_render_path_grid(
    const AntNetContext* ctx,
    const int* node_ids,
    int node_count,
    float offset_x,
    float offset_y,
    float* out_coords,
    int max_coords,
    int* out_count
);

#ifdef __cplusplus
}
#endif

#endif /* PATH_RENDERER_H */
