/* Relative Path: include/rendering/heatmap_renderer_async.h */
#ifndef HEATMAP_RENDERER_ASYNC_H
#define HEATMAP_RENDERER_ASYNC_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * hr_renderer_start
 * Creates a background rendering thread that owns one EGL/GLES context.
 * width/height are just the initial resolution hints; it can adapt on first job.
 * Returns 0 on success, negative on failure.
 */
int hr_renderer_start(int width, int height);

/*
 * hr_renderer_stop
 * Stops the background thread, joins it, and destroys the EGL context.
 * Returns 0 on success, negative on failure.
 */
int hr_renderer_stop(void);

/*
 * hr_enqueue_render
 * Blocks while rendering the given point set into out_rgba using the background thread.
 * Returns 0 on success, negative on error.
 */
int hr_enqueue_render(
    const float *pts_xy,
    const float *strength,
    int n,
    unsigned char *out_rgba,
    int width,
    int height
);

#ifdef __cplusplus
}
#endif

#endif /* HEATMAP_RENDERER_ASYNC_H */
