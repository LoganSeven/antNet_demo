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


#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef _WIN32
#include <pthread.h>
#endif



/*
 * pub_render_heatmap_rgba
 *
 * GPU-accelerated offscreen heatmap rendering based on a cloud of 2D points
 * and their corresponding pheromone strength values. This function is fully
 * decoupled from AntNetContext and may be called from any thread.
 * It uses a persistent background renderer (hr_renderer_async).
 */
int pub_render_heatmap_rgba(
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
 * pub_renderer_async_init
 * Starts the persistent renderer thread if not already running. Returns 0 on success.
 */
int pub_renderer_async_init(int initial_width, int initial_height)
{
    int ret = hr_renderer_start(initial_width, initial_height);
    if (ret != 0)
    {
        return ERR_INTERNAL_FAILURE;
    }
    return ERR_SUCCESS;
}

/*
 * pub_renderer_async_shutdown
 * Stops the background renderer thread if running, cleans up. 
 * Safe to call multiple times.
 */
int pub_renderer_async_shutdown(void)
{
    int ret = hr_renderer_stop();
    if (ret != 0)
    {
        return ERR_INTERNAL_FAILURE;
    }
    return ERR_SUCCESS;
}

