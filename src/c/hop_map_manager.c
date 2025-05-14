// src/c/hop_map_manager.c
#include "../../include/hop_map_manager.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/*
  Minimal reference implementation of a HopMapManager in C.
  Thread safety ensured via the lock mutex. This code is purely for demonstration.
  Integrate random seeding and advanced placement logic as needed.
*/

//static float clampf(float val, float minval, float maxval) {
//    return (val < minval) ? minval : (val > maxval ? maxval : val);
//}

/* We no longer need this for uniform approach, but keep if needed for fallback:
static float rand_gauss(float mean, float stddev) {
    float u1 = (float)rand() / (float)(RAND_MAX);
    float u2 = (float)rand() / (float)(RAND_MAX);
    float r  = sqrtf(-2.0f * logf(u1)) * stddev;
    float theta = 2.0f * 3.14159265f * u2;
    return mean + r * cosf(theta);
}
*/

HopMapManager* hop_map_manager_create() {
    HopMapManager *mgr = (HopMapManager*)malloc(sizeof(HopMapManager));
    if (!mgr) {
        return NULL;
    }

#ifndef _WIN32
    pthread_mutex_init(&mgr->lock, NULL);
#endif

    mgr->start_node = NULL;
    mgr->end_node   = NULL;
    mgr->hop_nodes  = NULL;
    mgr->hop_count  = 0;
    mgr->edges      = NULL;
    mgr->edge_count = 0;

    srand((unsigned int)time(NULL));

    return mgr;
}

void hop_map_manager_destroy(HopMapManager *mgr) {
    if (!mgr) return;

#ifndef _WIN32
    pthread_mutex_destroy(&mgr->lock);
#endif

    if (mgr->start_node) free(mgr->start_node);
    if (mgr->end_node)   free(mgr->end_node);
    if (mgr->hop_nodes)  free(mgr->hop_nodes);
    if (mgr->edges)      free(mgr->edges);

    free(mgr);
}

static float rand_uniform(float minval, float maxval) {
    float r = (float)rand() / (float)(RAND_MAX);
    return minval + r * (maxval - minval);
}

static int try_place_node_uniform(
    float *out_x, float *out_y,
    float margin, float width, float height,
    int radius,
    float *existing_x, float *existing_y,
    size_t existing_count
) {
    /*
     * Attempts to find a random uniform position for a node
     * that does not overlap with existing positions. Returns 1 if placed, 0 if fail.
     */
    int max_tries = 100;
    float required_spacing_sq = (radius * 2.0f + 30.0f) * (radius * 2.0f + 30.0f);

    for (int t = 0; t < max_tries; t++) {
        float x = rand_uniform(margin, width - margin);
        float y = rand_uniform(margin, height - margin);

        int ok = 1;
        for (size_t i = 0; i < existing_count; i++) {
            float dx = existing_x[i] - x;
            float dy = existing_y[i] - y;
            if ((dx*dx + dy*dy) < required_spacing_sq) {
                ok = 0;
                break;
            }
        }
        if (ok) {
            *out_x = x;
            *out_y = y;
            return 1;
        }
    }
    return 0;
}

void hop_map_manager_initialize_map(HopMapManager *mgr, int total_nodes) {
    if (!mgr) return;

#ifndef _WIN32
    pthread_mutex_lock(&mgr->lock);
#endif

    if (total_nodes < 2) {
        total_nodes = 2;
    }

    /* free old data */
    if (mgr->start_node) { free(mgr->start_node); mgr->start_node = NULL; }
    if (mgr->end_node)   { free(mgr->end_node);   mgr->end_node   = NULL; }
    if (mgr->hop_nodes)  { free(mgr->hop_nodes);  mgr->hop_nodes  = NULL; mgr->hop_count = 0; }
    if (mgr->edges)      { free(mgr->edges);      mgr->edges      = NULL; mgr->edge_count = 0; }

    mgr->start_node = (NodeData*)malloc(sizeof(NodeData));
    mgr->end_node   = (NodeData*)malloc(sizeof(NodeData));

    if (!mgr->start_node || !mgr->end_node) {
#ifndef _WIN32
        pthread_mutex_unlock(&mgr->lock);
#endif
        return; /* out of memory handling */
    }

    float width  = 1000.0f;
    float height = 600.0f;
    float margin = 50.0f;
    int radius   = 15;

    float mid_y = height / 2.0f;

    /* Start node */
    mgr->start_node->node_id  = 0;
    mgr->start_node->x        = margin;
    mgr->start_node->y        = mid_y;
    mgr->start_node->radius   = radius;
    mgr->start_node->delay_ms = (rand() % 41) + 10; /* random 10..50 */

    /* End node */
    mgr->end_node->node_id  = 1;
    mgr->end_node->x        = width - margin;
    mgr->end_node->y        = mid_y;
    mgr->end_node->radius   = radius;
    mgr->end_node->delay_ms = (rand() % 41) + 10; /* random 10..50 */

    /* Hop nodes */
    mgr->hop_count = (size_t)(total_nodes - 2);
    if (mgr->hop_count > 0) {
        mgr->hop_nodes = (NodeData*)malloc(sizeof(NodeData) * mgr->hop_count);
        if (!mgr->hop_nodes) {
#ifndef _WIN32
            pthread_mutex_unlock(&mgr->lock);
#endif
            return;
        }
    }

    /* Collect existing positions in arrays for quick checks: start + end at first */
    float *placed_x = (float*)malloc(sizeof(float) * (mgr->hop_count + 2));
    float *placed_y = (float*)malloc(sizeof(float) * (mgr->hop_count + 2));
    size_t placed_count = 0;
    if (placed_x && placed_y) {
        placed_x[placed_count] = mgr->start_node->x;
        placed_y[placed_count] = mgr->start_node->y;
        placed_count++;
        placed_x[placed_count] = mgr->end_node->x;
        placed_y[placed_count] = mgr->end_node->y;
        placed_count++;
    }

    for (size_t i = 0; i < mgr->hop_count; i++) {
        mgr->hop_nodes[i].node_id  = (int)(i + 2);
        mgr->hop_nodes[i].radius   = radius;
        mgr->hop_nodes[i].delay_ms = (rand() % 41) + 10;

        float x_final = width / 2.0f;
        float y_final = height / 2.0f;

        int success = 0;
        if (placed_x && placed_y) {
            /* Attempt uniform random placement */
            if (try_place_node_uniform(
                    &x_final, &y_final,
                    margin, width, height,
                    radius,
                    placed_x, placed_y,
                    placed_count
                ))
            {
                success = 1;
            }
        }

        if (!success) {
            /* Fallback if we never found a non-overlapping spot after max_tries */
            /* Place near corners with small local tries to avoid overlap */
            float corners[4][2] = {
                { margin, margin },
                { width - margin, margin },
                { margin, height - margin },
                { width - margin, height - margin }
            };
            /* Shuffle corner indices for more variability */
            int corner_indices[4] = {0, 1, 2, 3};
            for (int c = 0; c < 4; c++) {
                int swap_idx = c + rand() % (4 - c);
                int tmp = corner_indices[c];
                corner_indices[c] = corner_indices[swap_idx];
                corner_indices[swap_idx] = tmp;
            }

            int corner_placed = 0;
            float required_spacing_sq = (radius * 2.0f + 30.0f) * (radius * 2.0f + 30.0f);
            for (int c_idx = 0; c_idx < 4 && !corner_placed; c_idx++) {
                int idx = corner_indices[c_idx];
                float cx = corners[idx][0];
                float cy = corners[idx][1];

                /* Try up to 10 small offsets around the corner */
                for (int attempt = 0; attempt < 10; attempt++) {
                    float ox = cx + rand_uniform(-40.0f, 40.0f);
                    float oy = cy + rand_uniform(-40.0f, 40.0f);

                    int ok = 1;
                    for (size_t p = 0; p < placed_count; p++) {
                        float dx = placed_x[p] - ox;
                        float dy = placed_y[p] - oy;
                        if ((dx*dx + dy*dy) < required_spacing_sq) {
                            ok = 0;
                            break;
                        }
                    }
                    if (ok) {
                        x_final = ox;
                        y_final = oy;
                        corner_placed = 1;
                        success = 1;
                        break;
                    }
                }
            }

            if (!corner_placed) {
                /* If all else fails, place forcibly at the first corner (may cause overlap) */
                int idx = corner_indices[0];
                x_final = corners[idx][0];
                y_final = corners[idx][1];
                success = 1;
            }
        }

        mgr->hop_nodes[i].x = x_final;
        mgr->hop_nodes[i].y = y_final;

        /* If we successfully have placed_x/placed_y arrays, record this new position */
        if (placed_x && placed_y) {
            placed_x[placed_count] = x_final;
            placed_y[placed_count] = y_final;
            placed_count++;
        }
    }

    if (placed_x) free(placed_x);
    if (placed_y) free(placed_y);

#ifndef _WIN32
    pthread_mutex_unlock(&mgr->lock);
#endif
}

/* 
 * The rest (create_default_edges, hop_map_manager_export_topology, etc.)
 * remains unchanged from your original code. Shown for completeness:
 */

static float dist_sq(float ax, float ay, float bx, float by) {
    float dx = ax - bx;
    float dy = ay - by;
    return dx*dx + dy*dy;
}

void hop_map_manager_create_default_edges(HopMapManager *mgr) {
    if (!mgr) return;

#ifndef _WIN32
    pthread_mutex_lock(&mgr->lock);
#endif

    if (mgr->edges) {
        free(mgr->edges);
        mgr->edges = NULL;
    }
    mgr->edge_count = 0;

    if (!mgr->start_node || !mgr->end_node) {
#ifndef _WIN32
        pthread_mutex_unlock(&mgr->lock);
#endif
        return;
    }
    if (!mgr->hop_nodes || mgr->hop_count == 0) {
        mgr->edges = (EdgeData*)malloc(sizeof(EdgeData));
        if (mgr->edges) {
            mgr->edge_count = 1;
            mgr->edges[0].from_id = mgr->start_node->node_id;
            mgr->edges[0].to_id   = mgr->end_node->node_id;
        }
#ifndef _WIN32
        pthread_mutex_unlock(&mgr->lock);
#endif
        return;
    }

    size_t interior = (mgr->hop_count < 3) ? mgr->hop_count : 3;
    int *path_node_ids = (int*)malloc(sizeof(int) * (2 + interior));
    if (!path_node_ids) {
#ifndef _WIN32
        pthread_mutex_unlock(&mgr->lock);
#endif
        return;
    }

    path_node_ids[0] = mgr->start_node->node_id;
    int *hop_idx_used = (int*)calloc(mgr->hop_count, sizeof(int));
    if (!hop_idx_used) {
        free(path_node_ids);
#ifndef _WIN32
        pthread_mutex_unlock(&mgr->lock);
#endif
        return;
    }

    NodeData current = *(mgr->start_node);
    size_t path_index = 1;
    for (size_t step = 0; step < interior; step++) {
        float best_d = 1e12f;
        int best_idx = -1;
        for (size_t h = 0; h < mgr->hop_count; h++) {
            if (hop_idx_used[h]) continue;
            NodeData *cand = &mgr->hop_nodes[h];
            float dsq = dist_sq(current.x, current.y, cand->x, cand->y);
            if (dsq < best_d) {
                best_d = dsq;
                best_idx = (int)h;
            }
        }
        if (best_idx >= 0) {
            path_node_ids[path_index++] = mgr->hop_nodes[best_idx].node_id;
            hop_idx_used[best_idx] = 1;
            current = mgr->hop_nodes[best_idx];
        }
    }

    path_node_ids[path_index++] = mgr->end_node->node_id;
    size_t final_count = path_index;

    mgr->edge_count = final_count - 1;
    mgr->edges = (EdgeData*)malloc(sizeof(EdgeData) * mgr->edge_count);
    if (mgr->edges) {
        for (size_t i = 0; i < mgr->edge_count; i++) {
            mgr->edges[i].from_id = path_node_ids[i];
            mgr->edges[i].to_id   = path_node_ids[i+1];
        }
    }

    free(path_node_ids);
    free(hop_idx_used);

#ifndef _WIN32
    pthread_mutex_unlock(&mgr->lock);
#endif
}

void hop_map_manager_export_topology(HopMapManager *mgr,
                                     NodeData *out_nodes, size_t *out_node_count,
                                     EdgeData *out_edges, size_t *out_edge_count)
{
    if (!mgr) {
        if (out_node_count) *out_node_count = 0;
        if (out_edge_count) *out_edge_count = 0;
        return;
    }

#ifndef _WIN32
    pthread_mutex_lock(&mgr->lock);
#endif

    size_t ncount = 0;
    if (mgr->start_node) ncount++;
    if (mgr->end_node)   ncount++;
    ncount += mgr->hop_count;

    size_t ecount = mgr->edge_count;

    if (out_node_count) {
        if (out_nodes) {
            size_t idx = 0;
            if (mgr->start_node) {
                out_nodes[idx++] = *(mgr->start_node);
            }
            if (mgr->hop_nodes && mgr->hop_count > 0) {
                for (size_t i = 0; i < mgr->hop_count; i++) {
                    out_nodes[idx++] = mgr->hop_nodes[i];
                }
            }
            if (mgr->end_node) {
                out_nodes[idx++] = *(mgr->end_node);
            }
        }
        *out_node_count = ncount;
    }

    if (out_edge_count) {
        if (out_edges && mgr->edges && ecount > 0) {
            memcpy(out_edges, mgr->edges, sizeof(EdgeData) * ecount);
        }
        *out_edge_count = ecount;
    }

#ifndef _WIN32
    pthread_mutex_unlock(&mgr->lock);
#endif
}
