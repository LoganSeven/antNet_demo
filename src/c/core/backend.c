/* Relative Path: src/c/core/backend.c */
/*
 * backend.c
 * Now contains only the rendering functions and their async manager calls,
 * plus any other logic not covered by the initialization, solver, or parameter files.
 */
#include "../../../include/core/backend_init.h"
#include "../../../include/core/backend.h"
#include "../../../include/consts/error_codes.h"
#include "../../../include/rendering/heatmap_renderer_async.h"
#include "../../../include/rendering/path_renderer.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef _WIN32
#include <pthread.h>
#endif

extern int pr_render_path_grid(
    const AntNetContext* ctx,
    const int* node_ids,
    int node_count,
    float offset_x,
    float offset_y,
    float* out_coords,
    int max_coords,
    int* out_count
);

/*
 * antnet_render_heatmap_rgba
 *
 * GPU-accelerated offscreen heatmap rendering based on a cloud of 2D points
 * and their corresponding pheromone strength values. This function is fully
 * decoupled from AntNetContext and may be called from any thread.
 * It uses a persistent background renderer (hr_renderer_async).
 */
int antnet_render_heatmap_rgba(
    const float *pts_xy,
    const float *strength,
    int n,
    unsigned char *out_rgba,
    int width,
    int height
)
{
    if (!pts_xy || !strength || !out_rgba || n <= 0 || width <= 0 || height <= 0)
    {
        return ERR_INVALID_ARGS;
    }

    int rc = hr_enqueue_render(pts_xy, strength, n, out_rgba, width, height);
    if (rc != 0)
    {
        return ERR_INTERNAL_FAILURE;
    }
    return ERR_SUCCESS;
}

/*
 * antnet_renderer_async_init
 * Starts the persistent renderer thread if not already running. Returns 0 on success.
 */
int antnet_renderer_async_init(int initial_width, int initial_height)
{
    int ret = hr_renderer_start(initial_width, initial_height);
    if (ret != 0)
    {
        return ERR_INTERNAL_FAILURE;
    }
    return ERR_SUCCESS;
}

/*
 * antnet_renderer_async_shutdown
 * Stops the background renderer thread if running, cleans up. 
 * Safe to call multiple times.
 */
int antnet_renderer_async_shutdown(void)
{
    int ret = hr_renderer_stop();
    if (ret != 0)
    {
        return ERR_INTERNAL_FAILURE;
    }
    return ERR_SUCCESS;
}

int antnet_render_path_grid(
    int context_id,
    const int* node_ids,
    int node_count,
    float offset_x,
    float offset_y,
    float* out_coords,
    int max_coords,
    int* out_count
)
{
    if (!node_ids || node_count < 2 || !out_coords || max_coords < 2 || !out_count) {
        return ERR_INVALID_ARGS;
    }
    AntNetContext* ctx = get_context_by_id(context_id);
    if (!ctx) {
        return ERR_INVALID_CONTEXT;
    }

#ifndef _WIN32
    pthread_mutex_lock(&ctx->lock);
#endif

    /* Delegates to the internal path_renderer logic */
    int rc = pr_render_path_grid(ctx, node_ids, node_count,
                             offset_x, offset_y,
                             out_coords, max_coords, out_count);

#ifndef _WIN32
    pthread_mutex_unlock(&ctx->lock);
#endif

    if (rc == 0) {
        return ERR_SUCCESS;
    }
    return ERR_INTERNAL_FAILURE;
}
