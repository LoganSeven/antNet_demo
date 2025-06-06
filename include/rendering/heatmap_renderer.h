/* Relative Path: include/rendering/heatmap_renderer.h */
/*
 * Defines an offscreen HeatmapRenderer and its basic operations.
 * Responsible for creating/destroying a rendering instance and drawing a heatmap.
 * Low-level interface used by the asynchronous layer for actual GPU-based computations.
*/

#ifndef HEATMAP_RENDERER_H
#define HEATMAP_RENDERER_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct HeatmapRenderer HeatmapRenderer;

HeatmapRenderer *hr_create(int width, int height);
int hr_render(HeatmapRenderer *hr,
              const float *pts,
              const float *strength,
              size_t n,
              unsigned char *out_rgba,
              int width,
              int height);
void hr_destroy(HeatmapRenderer *hr);

#ifdef __cplusplus
}
#endif

#endif
