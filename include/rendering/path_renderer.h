/* Relative Path: include/rendering/path_renderer.h */
#include "../../include/core/backend.h" /* for get_context_by_id(...) */
#ifndef PATH_RENDERER_H
#define PATH_RENDERER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * antnet_render_path_grid
 *
 * Computes a 90-degree segmented path for the given list of node IDs
 * and writes the resulting float (x,y) coordinates into out_coords.
 *
 * Each pair of adjacent node_ids is routed on a discrete grid,
 * avoiding other nodes as obstacles. The final path is returned
 * as a list of consecutive (x,y) points forming a polyline.
 *
 * Thread-safe usage requires external locking of the context by the caller.
 *
 * @param context_id  - context handle
 * @param node_ids    - array of node IDs forming the path
 * @param node_count  - number of node IDs in node_ids
 * @param offset_x    - horizontal visual offset for all returned coordinates
 * @param offset_y    - vertical visual offset for all returned coordinates
 * @param out_coords  - preallocated float array where (x,y) pairs will be written
 * @param max_coords  - capacity of out_coords in number of floats
 * @param out_count   - output: number of floats actually written to out_coords (pairs * 2)
 *
 * @return 0 on success, non-zero on failure (no path, insufficient space, etc.)
 */
int antnet_render_path_grid(
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
 * Internal rendering function used by antnet_render_path_grid().
 * Assumes the context is already locked.
 */
int pr_render_path_grid(
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
