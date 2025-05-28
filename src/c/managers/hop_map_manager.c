/* Relative Path: src/c/managers/hop_map_manager.c */
/*
 * Builds and maintains a hop-based node map, assigning positions and delays.
 * Creates default edges and exports topology data in a thread-safe manner.
 * Useful for demonstration or simple graph generation within AntNet.
 *
 * Now also exposes pub_hop_map_* functions that operate on the HopMapManager
 * stored inside AntNetContext->hop_map_mgr, consolidating node/edge creation
 * exclusively in the C side.
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#include "../../../include/managers/hop_map_manager.h"
#include "../../../include/consts/error_codes.h"
#include "../../../include/core/backend_init.h"       /* priv_get_context_by_id */
#include "../../../include/managers/hop_map_manager.h"/* HopMapManager, NodeData, EdgeData */

/*
  Minimal reference implementation of a HopMapManager in C.
  Thread-safety ensured via the lock mutex. This code is purely for demonstration.
  Integrate random seeding and advanced placement logic as needed.
*/

/*
 * Internal helper: distance squared
 */
static float dist_sq(float ax, float ay, float bx, float by) {
    float dx = ax - bx;
    float dy = ay - by;
    return dx*dx + dy*dy;
}

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

    /* default random delay range */
    mgr->default_min_delay = 10;
    mgr->default_max_delay = 50;

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
 * Allows setting the random delay range for node latencies in a thread-safe manner.
 */
void hop_map_manager_set_delay_range(HopMapManager *mgr, int min_delay, int max_delay) {
    if (!mgr) return;
#ifndef _WIN32
    pthread_mutex_lock(&mgr->lock);
#endif

    mgr->default_min_delay = (min_delay < 0) ? 0 : min_delay;
    mgr->default_max_delay = (max_delay < 0) ? 0 : max_delay;
    if (mgr->default_max_delay < mgr->default_min_delay) {
        mgr->default_max_delay = mgr->default_min_delay;
    }

#ifndef _WIN32
    pthread_mutex_unlock(&mgr->lock);
#endif
}

/*
 * Returns a random delay within the configured range.
 */
static int hop_map_manager_get_random_delay(const HopMapManager *mgr) {
    int range = mgr->default_max_delay - mgr->default_min_delay + 1;
    if (range < 1) {
        return mgr->default_min_delay;
    }
    int rnd = rand() % range;
    return mgr->default_min_delay + rnd;
}

/*
 * hop_map_manager_initialize_map
 * Allocates new node arrays for start, end, and hops if total_nodes changed.
 * If total_nodes is unchanged, it returns immediately (skips re-randomizing latencies).
 */
void hop_map_manager_initialize_map(HopMapManager *mgr, int total_nodes) {
    if (!mgr) return;

#ifndef _WIN32
    pthread_mutex_lock(&mgr->lock);
#endif

    size_t current_count = 0;
    if (mgr->start_node) current_count++;
    if (mgr->end_node)   current_count++;
    current_count += mgr->hop_count;

    if ((int)current_count == total_nodes) {
#ifndef _WIN32
        pthread_mutex_unlock(&mgr->lock);
#endif
        return;
    }

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
        return;
    }

    float width  = 1000.0f;
    float height = 600.0f;
    float margin = 50.0f;
    int radius   = 15;
    float mid_y  = height / 2.0f;

    /* Start node */
    mgr->start_node->node_id  = 0;
    mgr->start_node->x        = margin;
    mgr->start_node->y        = mid_y;
    mgr->start_node->radius   = radius;
    mgr->start_node->delay_ms = hop_map_manager_get_random_delay(mgr);

    /* End node */
    mgr->end_node->node_id  = 1;
    mgr->end_node->x        = width - margin;
    mgr->end_node->y        = mid_y;
    mgr->end_node->radius   = radius;
    mgr->end_node->delay_ms = hop_map_manager_get_random_delay(mgr);

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

    /* Lay out hops in a grid between start and end horizontally */
    if (mgr->hop_count > 0) {
        float grid_left   = mgr->start_node->x + 100.0f;
        float grid_right  = mgr->end_node->x   - 100.0f;
        if (grid_right < grid_left) {
            grid_left  = mgr->start_node->x;
            grid_right = mgr->start_node->x;
        }

        float grid_top    = 50.0f;
        float grid_bottom = height - 50.0f;
        int total_hops = (int)mgr->hop_count;

        float rowsf = sqrtf((float)total_hops);
        int row_count = (int)ceilf(rowsf);
        int col_count = (int)ceilf((float)total_hops / (float)row_count);

        float w = (grid_right  > grid_left) ? (grid_right  - grid_left) : 1.0f;
        float h = (grid_bottom > grid_top ) ? (grid_bottom - grid_top ) : 1.0f;

        float cell_width  = w / (float)col_count;
        float cell_height = h / (float)row_count;

        for (int i = 0; i < total_hops; i++) {
            mgr->hop_nodes[i].node_id  = i + 2;
            mgr->hop_nodes[i].radius   = radius;
            mgr->hop_nodes[i].delay_ms = hop_map_manager_get_random_delay(mgr);

            int row = i / col_count;
            int col = i % col_count;

            float cx = grid_left + (col + 0.5f) * cell_width;
            float cy = grid_top  + (row + 0.5f) * cell_height;

            mgr->hop_nodes[i].x = cx;
            mgr->hop_nodes[i].y = cy;
        }
    }

#ifndef _WIN32
    pthread_mutex_unlock(&mgr->lock);
#endif
}

/*
 * hop_map_manager_recalc_positions
 * This re-lays out the *existing* start_node, end_node, and hop_nodes
 * within the specified scene_width and scene_height. The function
 * does not modify node_id or delay_ms.
 */
void hop_map_manager_recalc_positions(HopMapManager *mgr,
                                      float scene_width,
                                      float scene_height)
{
    if (!mgr) return;

#ifndef _WIN32
    pthread_mutex_lock(&mgr->lock);
#endif

    if (scene_width <= 10.0f || scene_height <= 10.0f) {
#ifndef _WIN32
        pthread_mutex_unlock(&mgr->lock);
#endif
        return;
    }

    float margin = 50.0f;
    int radius = 15;

    if (mgr->start_node) {
        mgr->start_node->x = margin;
        mgr->start_node->y = scene_height * 0.5f;
    }

    if (mgr->end_node) {
        mgr->end_node->x = scene_width - margin;
        mgr->end_node->y = scene_height * 0.5f;
    }

    if (!mgr->hop_nodes || mgr->hop_count == 0) {
#ifndef _WIN32
        pthread_mutex_unlock(&mgr->lock);
#endif
        return;
    }

    float grid_left  = (mgr->start_node) ? (mgr->start_node->x + 100.0f) : 100.0f;
    float grid_right = (mgr->end_node)   ? (mgr->end_node->x   - 100.0f) : (scene_width - 100.0f);
    if (grid_right < grid_left) {
        grid_left = margin;
        grid_right = margin;
    }

    int total_hops = (int)mgr->hop_count;
    float rowsf = sqrtf((float)total_hops);
    int row_count = (int)ceilf(rowsf);
    int col_count = (int)ceilf((float)total_hops / (float)row_count);

    float usable_height = scene_height - 2.0f * margin;
    if (usable_height < 1.0f) {
#ifndef _WIN32
        pthread_mutex_unlock(&mgr->lock);
#endif
        return;
    }

    float cell_height = usable_height / (float)row_count;
    if (cell_height < (2.0f * radius)) {
        cell_height = 2.0f * radius;
    }

    float grid_total_height = row_count * cell_height;
    float top_offset = 0.5f * (scene_height - grid_total_height);
    if (top_offset < margin) {
        top_offset = margin;
    }

    float w = (grid_right > grid_left) ? (grid_right - grid_left) : 1.0f;
    if (col_count < 1) col_count = 1;
    float cell_width = w / (float)col_count;
    if (cell_width < (2.0f * radius)) {
        cell_width = 2.0f * radius;
    }

    for (int i = 0; i < total_hops; i++) {
        int row = i / col_count;
        int col = i % col_count;

        float cx = grid_left + (col + 0.5f) * cell_width;
        float cy = top_offset + (row + 0.5f) * cell_height;

        mgr->hop_nodes[i].x = cx;
        mgr->hop_nodes[i].y = cy;
    }

#ifndef _WIN32
    pthread_mutex_unlock(&mgr->lock);
#endif
}

/*
 * Creates default edges for up to 3 nearest hops, modifies mgr->edges
 */
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

    int *hop_idx_used = (int*)calloc(mgr->hop_count, sizeof(int));
    if (!hop_idx_used) {
        free(path_node_ids);
#ifndef _WIN32
        pthread_mutex_unlock(&mgr->lock);
#endif
        return;
    }

    path_node_ids[0] = mgr->start_node->node_id;
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
            mgr->edges[i].to_id   = path_node_ids[i + 1];
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
        if (out_nodes && ncount > 0) {
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
        if (out_edges && ecount > 0 && mgr->edges) {
            memcpy(out_edges, mgr->edges, sizeof(EdgeData) * ecount);
        }
        *out_edge_count = ecount;
    }

#ifndef _WIN32
    pthread_mutex_unlock(&mgr->lock);
#endif
}

/* ------------------------------------------------------------------
 *                 New public 'pub_' wrappers
 * ------------------------------------------------------------------
 *
 * Each function locks the same AntNetContext->lock used by the solver,
 * then calls the HopMapManager. This ensures thread safety across modules.
*/

/*
 * pub_hop_map_set_delay_range
 * Applies the given min/max to the context's HopMapManager in a thread-safe manner.
 */
int pub_hop_map_set_delay_range(int context_id, int min_d, int max_d)
{
    AntNetContext* ctx = priv_get_context_by_id(context_id);
    if (!ctx) {
        return ERR_INVALID_CONTEXT;
    }
#ifndef _WIN32
    pthread_mutex_lock(&ctx->lock);
#endif
    if (!ctx->hop_map_mgr) {
        /* Create if missing. This is optional logic. */
        ctx->hop_map_mgr = hop_map_manager_create();
        if (!ctx->hop_map_mgr) {
#ifndef _WIN32
            pthread_mutex_unlock(&ctx->lock);
#endif
            return ERR_MEMORY_ALLOCATION;
        }
    }
    /* HopMapManager usage is also locked internally, but we hold this outer
     * lock to keep everything consistent if solver or other calls happen. */
    hop_map_manager_set_delay_range(ctx->hop_map_mgr, min_d, max_d);

#ifndef _WIN32
    pthread_mutex_unlock(&ctx->lock);
#endif
    return ERR_SUCCESS;
}

/*
 * pub_hop_map_initialize
 * Initializes or re-initializes the HopMapManager with total_nodes.
 */
int pub_hop_map_initialize(int context_id, int total_nodes)
{
    AntNetContext* ctx = priv_get_context_by_id(context_id);
    if (!ctx) {
        return ERR_INVALID_CONTEXT;
    }

#ifndef _WIN32
    pthread_mutex_lock(&ctx->lock);
#endif
    if (!ctx->hop_map_mgr) {
        ctx->hop_map_mgr = hop_map_manager_create();
        if (!ctx->hop_map_mgr) {
#ifndef _WIN32
            pthread_mutex_unlock(&ctx->lock);
#endif
            return ERR_MEMORY_ALLOCATION;
        }
    }
#ifndef _WIN32
    pthread_mutex_unlock(&ctx->lock);
#endif

    /* Now call the manager's initialize_map inside the same lock. */
    hop_map_manager_initialize_map(ctx->hop_map_mgr, total_nodes);

    return ERR_SUCCESS;
}

/*
 * pub_hop_map_create_default_edges
 */
int pub_hop_map_create_default_edges(int context_id)
{
    AntNetContext* ctx = priv_get_context_by_id(context_id);
    if (!ctx) {
        return ERR_INVALID_CONTEXT;
    }

#ifndef _WIN32
    pthread_mutex_lock(&ctx->lock);
#endif
    if (!ctx->hop_map_mgr) {
#ifndef _WIN32
        pthread_mutex_unlock(&ctx->lock);
#endif
        return ERR_NO_TOPOLOGY;
    }
#ifndef _WIN32
    pthread_mutex_unlock(&ctx->lock);
#endif

    hop_map_manager_create_default_edges(ctx->hop_map_mgr);
    return ERR_SUCCESS;
}

/*
 * pub_hop_map_recalc_positions
 * Re-lays out existing nodes for the new scene size.
 */
int pub_hop_map_recalc_positions(int context_id, float scene_w, float scene_h)
{
    AntNetContext* ctx = priv_get_context_by_id(context_id);
    if (!ctx) {
        return ERR_INVALID_CONTEXT;
    }

#ifndef _WIN32
    pthread_mutex_lock(&ctx->lock);
#endif
    if (!ctx->hop_map_mgr) {
#ifndef _WIN32
        pthread_mutex_unlock(&ctx->lock);
#endif
        return ERR_NO_TOPOLOGY;
    }
#ifndef _WIN32
    pthread_mutex_unlock(&ctx->lock);
#endif

    hop_map_manager_recalc_positions(ctx->hop_map_mgr, scene_w, scene_h);
    return ERR_SUCCESS;
}

/*
 * pub_hop_map_export_topology
 * Exports the current HopMapManager node+edge data to the caller.
 * The caller supplies out_nodes/out_node_count, out_edges/out_edge_count, just
 * like pub_update_topology does for the solver. Returns 0 or negative on error.
 */
int pub_hop_map_export_topology(int context_id,
                                NodeData *out_nodes, int *out_node_count,
                                EdgeData *out_edges, int *out_edge_count)
{
    if (!out_node_count || !out_edge_count) {
        return ERR_INVALID_ARGS;
    }

    AntNetContext* ctx = priv_get_context_by_id(context_id);
    if (!ctx) {
        return ERR_INVALID_CONTEXT;
    }

#ifndef _WIN32
    pthread_mutex_lock(&ctx->lock);
#endif
    if (!ctx->hop_map_mgr) {
        /* 0 nodes/edges if not inited */
        if (out_node_count) *out_node_count = 0;
        if (out_edge_count) *out_edge_count = 0;
#ifndef _WIN32
        pthread_mutex_unlock(&ctx->lock);
#endif
        return ERR_NO_TOPOLOGY;
    }
#ifndef _WIN32
    pthread_mutex_unlock(&ctx->lock);
#endif

    size_t node_count_sz = 0;
    size_t edge_count_sz = 0;
    hop_map_manager_export_topology(ctx->hop_map_mgr,
                                    out_nodes, &node_count_sz,
                                    out_edges, &edge_count_sz);

    if (out_node_count) *out_node_count = (int)node_count_sz;
    if (out_edge_count) *out_edge_count = (int)edge_count_sz;

    return ERR_SUCCESS;
}
