/* Relative Path: src/c/managers/hop_map_manager.c */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#include "../../../include/core/backend.h"
#include "../../../include/core/backend_topology.h"
#include "../../../include/consts/error_codes.h"
#include "../../../include/managers/hop_map_manager.h"

/*
  Minimal reference implementation of a HopMapManager in C.
  Thread safety ensured via the lock mutex. This code is purely for demonstration.
  Integrate random seeding and advanced placement logic as needed.
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

/*
 * hop_map_manager_initialize_map
 * Keeps the start/end node in the same place as before, then arranges
 * the hop nodes in a clean 2D grid between them. The randomness is only
 * in the node latency, not in their positions.
 */
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

    /* Start node (unchanged) */
    mgr->start_node->node_id  = 0;
    mgr->start_node->x        = margin;
    mgr->start_node->y        = mid_y;
    mgr->start_node->radius   = radius;
    mgr->start_node->delay_ms = (rand() % 41) + 10; /* random 10..50 */

    /* End node (unchanged) */
    mgr->end_node->node_id  = 1;
    mgr->end_node->x        = width - margin;
    mgr->end_node->y        = mid_y;
    mgr->end_node->radius   = radius;
    mgr->end_node->delay_ms = (rand() % 41) + 10; /* random 10..50 */

    /* Hop nodes in a grid between start and end */
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

    /* Compute a bounding box for the grid (between start.x+100 and end.x-100, top to bottom) */
    float grid_left   = mgr->start_node->x + 100.0f;
    float grid_right  = mgr->end_node->x   - 100.0f;
    float grid_top    = margin;
    float grid_bottom = height - margin;

    if (grid_right < grid_left) {
        /* If total_nodes is large, or margin is huge, gracefully skip. */
        grid_left  = mgr->start_node->x;
        grid_right = mgr->start_node->x;
    }

    int total_hops = (int)mgr->hop_count;
    if (total_hops > 0) {
        float rowsf = sqrtf((float)total_hops);
        int row_count = (int)ceilf(rowsf);
        int col_count = (int)ceilf((float)total_hops / (float)row_count);

        /* If grid_right <= grid_left, avoid divide by zero */
        float w = (grid_right  > grid_left) ? (grid_right  - grid_left) : 1.0f;
        float h = (grid_bottom > grid_top ) ? (grid_bottom - grid_top ) : 1.0f;

        float cell_width  = w / (float)col_count;
        float cell_height = h / (float)row_count;

        for (int i = 0; i < total_hops; i++) {
            int row = i / col_count;
            int col = i % col_count;

            float cx = grid_left + (col + 0.5f) * cell_width;
            float cy = grid_top  + (row + 0.5f) * cell_height;

            mgr->hop_nodes[i].node_id  = (int)(i + 2);
            mgr->hop_nodes[i].x        = cx;
            mgr->hop_nodes[i].y        = cy;
            mgr->hop_nodes[i].radius   = radius;
            mgr->hop_nodes[i].delay_ms = (rand() % 41) + 10; /* random 10..50 */
        }
    }

#ifndef _WIN32
    pthread_mutex_unlock(&mgr->lock);
#endif
}

/*
 * The rest (create_default_edges, hop_map_manager_export_topology, etc.)
 * remains unchanged from your original code. Shown here for completeness:
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
