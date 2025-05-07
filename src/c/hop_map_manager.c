#include "hop_map_manager.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/*
  Minimal reference implementation of a HopMapManager in C.
  Thread safety ensured via the lock mutex. This code is purely for demonstration.
  Integrate random seeding and advanced placement logic as needed.
*/

static float clampf(float val, float minval, float maxval) {
    return (val < minval) ? minval : (val > maxval ? maxval : val);
}

static float rand_gauss(float mean, float stddev) {
    /* Using Box-Muller transform for approximate Gaussian random */
    float u1 = (float)rand() / (float)(RAND_MAX);
    float u2 = (float)rand() / (float)(RAND_MAX);
    float r  = sqrtf(-2.0f * logf(u1)) * stddev;
    float theta = 2.0f * 3.14159265f * u2;
    return mean + r * cosf(theta);
}

HopMapManager* hop_map_manager_create() {
    HopMapManager *mgr = (HopMapManager*)malloc(sizeof(HopMapManager));
    if (!mgr) {
        return NULL;
    }
    pthread_mutex_init(&mgr->lock, NULL);
    mgr->start_node = NULL;
    mgr->end_node   = NULL;
    mgr->hop_nodes  = NULL;
    mgr->hop_count  = 0;
    mgr->edges      = NULL;
    mgr->edge_count = 0;

    /* Optionally seed the RNG once per manager instance if not done globally */
    srand((unsigned int)time(NULL));

    return mgr;
}

void hop_map_manager_destroy(HopMapManager *mgr) {
    if (!mgr) return;
    pthread_mutex_destroy(&mgr->lock);

    if (mgr->start_node) free(mgr->start_node);
    if (mgr->end_node)   free(mgr->end_node);
    if (mgr->hop_nodes)  free(mgr->hop_nodes);
    if (mgr->edges)      free(mgr->edges);

    free(mgr);
}

void hop_map_manager_initialize_map(HopMapManager *mgr, int total_nodes) {
    if (!mgr) return;
    pthread_mutex_lock(&mgr->lock);

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

    /* Basic geometry placeholders as in the Python version */
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
    mgr->hop_nodes = (NodeData*)malloc(sizeof(NodeData) * mgr->hop_count);
    if (!mgr->hop_nodes) {
        pthread_mutex_unlock(&mgr->lock);
        return;
    }

    float spacing_sq = (radius * 2.0f + 20.0f)*(radius * 2.0f + 20.0f);
    for (size_t i = 0; i < mgr->hop_count; i++) {
        mgr->hop_nodes[i].node_id = (int)(i + 2);
        mgr->hop_nodes[i].radius  = radius;
        mgr->hop_nodes[i].delay_ms = (rand() % 41) + 10; /* random 10..50 */

        /* Attempt random placement */
        int max_tries = 100;
        int placed = 0;
        for (int t = 0; t < max_tries; t++) {
            float x = rand_gauss(width / 2.0f, width / 6.0f);
            float y = rand_gauss(height / 2.0f, height / 6.0f);

            x = clampf(x, margin, width - margin);
            y = clampf(y, margin, height - margin);

            /* check spacing from start/end/hops placed so far */
            int ok = 1;
            {
                float dx = mgr->start_node->x - x;
                float dy = mgr->start_node->y - y;
                if ((dx*dx + dy*dy) < spacing_sq) ok = 0;
            }
            if (ok) {
                float dx = mgr->end_node->x - x;
                float dy = mgr->end_node->y - y;
                if ((dx*dx + dy*dy) < spacing_sq) ok = 0;
            }
            if (ok) {
                for (size_t j = 0; j < i; j++) {
                    float dx = mgr->hop_nodes[j].x - x;
                    float dy = mgr->hop_nodes[j].y - y;
                    if ((dx*dx + dy*dy) < spacing_sq) {
                        ok = 0;
                        break;
                    }
                }
            }

            if (ok) {
                mgr->hop_nodes[i].x = x;
                mgr->hop_nodes[i].y = y;
                placed = 1;
                break;
            }
        }

        if (!placed) {
            /* fallback center */
            mgr->hop_nodes[i].x = width / 2.0f;
            mgr->hop_nodes[i].y = height / 2.0f;
        }
    }

    pthread_mutex_unlock(&mgr->lock);
}

static float dist_sq(float ax, float ay, float bx, float by) {
    float dx = ax - bx;
    float dy = ay - by;
    return dx*dx + dy*dy;
}

void hop_map_manager_create_default_edges(HopMapManager *mgr) {
    if (!mgr) return;
    pthread_mutex_lock(&mgr->lock);

    /* clear old edges */
    if (mgr->edges) {
        free(mgr->edges);
        mgr->edges = NULL;
    }
    mgr->edge_count = 0;

    if (!mgr->start_node || !mgr->end_node) {
        pthread_mutex_unlock(&mgr->lock);
        return;
    }
    if (!mgr->hop_nodes || mgr->hop_count == 0) {
        /* can simply connect start->end */
        mgr->edges = (EdgeData*)malloc(sizeof(EdgeData));
        mgr->edge_count = 1;
        mgr->edges[0].from_id = mgr->start_node->node_id;
        mgr->edges[0].to_id   = mgr->end_node->node_id;
        pthread_mutex_unlock(&mgr->lock);
        return;
    }

    /* up to 3 nearest hops in sequence from start to end */
    size_t interior = (mgr->hop_count < 3) ? mgr->hop_count : 3;
    int *path_node_ids = (int*)malloc(sizeof(int) * (2 + interior)); /* start + interior + end */
    if (!path_node_ids) {
        pthread_mutex_unlock(&mgr->lock);
        return;
    }

    path_node_ids[0] = mgr->start_node->node_id;
    /* gather local array of hop indexes */
    size_t hop_count_local = mgr->hop_count;
    int *hop_idx_used = (int*)calloc(hop_count_local, sizeof(int)); /* track used hops */

    /* pick nearest each time */
    NodeData current = *(mgr->start_node);
    size_t path_index = 1;
    for (size_t step = 0; step < interior; step++) {
        float best_d = 1e12f;
        int best_idx = -1;
        for (size_t h = 0; h < hop_count_local; h++) {
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

    /* build edges from these node IDs */
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

    pthread_mutex_unlock(&mgr->lock);
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

    pthread_mutex_lock(&mgr->lock);

    /* Count how many nodes exist (start + end + hops) */
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

    pthread_mutex_unlock(&mgr->lock);
}
