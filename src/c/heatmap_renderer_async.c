/* src/c/heatmap_renderer_async.c */
#include "../../include/heatmap_renderer_async.h"
#include "../../include/heatmap_renderer.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/*
 * This file implements a single persistent background thread that owns an EGL + OpenGL ES 3
 * context. It accepts rendering jobs via hr_enqueue_render(...), which blocks until completion.
 * The actual rendering is delegated to the hr_render(...) function in heatmap_renderer.c.
 */

/* Holds state for one pending render job */
typedef struct RenderJob {
    float           *pts_xy;
    float           *strength;
    int             n;
    unsigned char   *out_rgba;
    int             width;
    int             height;
    bool            in_use;
    bool            done;
} RenderJob;

/* Internal structure for the async renderer thread */
static struct {
    pthread_t       thread;
    pthread_mutex_t lock;
    pthread_cond_t  cond;
    bool            running;

    /* The single internal renderer. Recreated on size change. */
    HeatmapRenderer *hr;
    int             last_w;
    int             last_h;

    /* The active job (we only support one at a time for simplicity). */
    RenderJob       job;
} g_render_state = {
    .thread     = 0,
    .lock       = PTHREAD_MUTEX_INITIALIZER,
    .cond       = PTHREAD_COND_INITIALIZER,
    .running    = false,
    .hr         = NULL,
    .last_w     = 0,
    .last_h     = 0
};

/*
 * Internal thread function. Waits for a job to appear, then processes it,
 * signals completion, and loops. Exits when running == false.
 */
static void *hr_render_thread_main(void *arg)
{
    (void)arg; /* unused */

    pthread_mutex_lock(&g_render_state.lock);
    while (g_render_state.running) {
        /* Wait for a valid job */
        while (g_render_state.running && !g_render_state.job.in_use) {
            pthread_cond_wait(&g_render_state.cond, &g_render_state.lock);
        }
        if (!g_render_state.running) {
            break;
        }

        /* We have a job. Recreate the internal renderer if size changed. */
        if (!g_render_state.hr ||
            g_render_state.last_w != g_render_state.job.width ||
            g_render_state.last_h != g_render_state.job.height)
        {
            if (g_render_state.hr) {
                hr_destroy(g_render_state.hr);
                g_render_state.hr = NULL;
            }
            g_render_state.hr = hr_create(g_render_state.job.width,
                                          g_render_state.job.height);
            g_render_state.last_w = g_render_state.job.width;
            g_render_state.last_h = g_render_state.job.height;
        }

        if (g_render_state.hr) {
            (void)hr_render(
                g_render_state.hr,
                g_render_state.job.pts_xy,
                g_render_state.job.strength,
                (size_t)g_render_state.job.n,
                g_render_state.job.out_rgba,
                g_render_state.job.width,
                g_render_state.job.height
            );
        }

        /* Mark completion */
        g_render_state.job.done  = true;
        g_render_state.job.in_use = false;
        free(g_render_state.job.pts_xy);
        free(g_render_state.job.strength);
        g_render_state.job.pts_xy = NULL;
        g_render_state.job.strength = NULL;

        pthread_cond_broadcast(&g_render_state.cond);
    }
    pthread_mutex_unlock(&g_render_state.lock);

    return NULL;
}

/*
 * hr_renderer_start
 * Creates the render thread, which will own a persistent EGL context.
 * If already running, returns 0 immediately.
 * width/height are the initial rendering size, can still be changed on the first job.
 *
 * Returns 0 on success, negative on error.
 */
int hr_renderer_start(int width, int height)
{
    pthread_mutex_lock(&g_render_state.lock);
    if (g_render_state.running) {
        /* Already running */
        pthread_mutex_unlock(&g_render_state.lock);
        return 0;
    }
    g_render_state.running = true;
    g_render_state.last_w  = width;
    g_render_state.last_h  = height;
    g_render_state.hr      = NULL;
    memset(&g_render_state.job, 0, sizeof(g_render_state.job));

    int rc = pthread_create(&g_render_state.thread, NULL,
                            hr_render_thread_main, NULL);
    if (rc != 0) {
        g_render_state.running = false;
        pthread_mutex_unlock(&g_render_state.lock);
        return -1;
    }
    pthread_mutex_unlock(&g_render_state.lock);

    return 0;
}

/*
 * hr_renderer_stop
 * Signals the thread to stop and joins it. Destroys the internal renderer.
 * Safe to call multiple times. Not thread-safe with hr_enqueue_render though.
 * Make sure no new jobs are being enqueued.
 */
int hr_renderer_stop(void)
{
    pthread_mutex_lock(&g_render_state.lock);
    if (!g_render_state.running) {
        pthread_mutex_unlock(&g_render_state.lock);
        return 0;
    }
    g_render_state.running = false;
    pthread_cond_broadcast(&g_render_state.cond);
    pthread_mutex_unlock(&g_render_state.lock);

    pthread_join(g_render_state.thread, NULL);
    pthread_mutex_lock(&g_render_state.lock);
    if (g_render_state.hr) {
        hr_destroy(g_render_state.hr);
        g_render_state.hr = NULL;
    }
    memset(&g_render_state.job, 0, sizeof(g_render_state.job));
    pthread_mutex_unlock(&g_render_state.lock);

    return 0;
}

/*
 * hr_enqueue_render
 * Enqueues one blocking render job. Copies 'pts_xy' and 'strength' internally.
 * Blocks until the job is finished, then returns 0 on success, negative on error.
 */
int hr_enqueue_render(
    const float *pts_xy,
    const float *strength,
    int n,
    unsigned char *out_rgba,
    int width,
    int height
)
{
    if (!pts_xy || !strength || !out_rgba || n <= 0 ||
        width <= 0 || height <= 0)
    {
        return -1;
    }

    /* Acquire the lock, fill in the job, notify the thread, wait for completion. */
    pthread_mutex_lock(&g_render_state.lock);

    if (!g_render_state.running) {
        pthread_mutex_unlock(&g_render_state.lock);
        return -2; /* not running */
    }

    /* If a job is in progress, wait for it to finish. */
    while (g_render_state.job.in_use && !g_render_state.job.done) {
        pthread_cond_wait(&g_render_state.cond, &g_render_state.lock);
    }

    /* Set up the new job */
    g_render_state.job.n        = n;
    g_render_state.job.width    = width;
    g_render_state.job.height   = height;
    g_render_state.job.out_rgba = out_rgba;
    g_render_state.job.done     = false;

    /* Copy data arrays. */
    size_t pts_size = (size_t)n * 2 * sizeof(float);
    size_t str_size = (size_t)n     * sizeof(float);
    g_render_state.job.pts_xy   = (float*)malloc(pts_size);
    g_render_state.job.strength = (float*)malloc(str_size);
    if (!g_render_state.job.pts_xy || !g_render_state.job.strength) {
        if (g_render_state.job.pts_xy)   free(g_render_state.job.pts_xy);
        if (g_render_state.job.strength) free(g_render_state.job.strength);
        g_render_state.job.pts_xy   = NULL;
        g_render_state.job.strength = NULL;
        pthread_mutex_unlock(&g_render_state.lock);
        return -3; /* mem fail */
    }

    memcpy(g_render_state.job.pts_xy, pts_xy, pts_size);
    memcpy(g_render_state.job.strength, strength, str_size);

    g_render_state.job.in_use = true;
    pthread_cond_broadcast(&g_render_state.cond);

    /* Wait for job.done */
    while (!g_render_state.job.done) {
        pthread_cond_wait(&g_render_state.cond, &g_render_state.lock);
    }

    pthread_mutex_unlock(&g_render_state.lock);
    return 0;
}
