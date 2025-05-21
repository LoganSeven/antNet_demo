/* Relative Path: src/c/rendering/path_renderer.c */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#include "../../../include/core/backend_init.h" /* for get_context_by_id(...) */
#include "../../../include/types/antnet_network_types.h"
#include "../../../include/rendering/path_renderer.h"

/* ---------------------------------------------------------
   Internal configuration for the routing grid
   --------------------------------------------------------- */

#define GRID_WIDTH  19
#define GRID_HEIGHT 19

/* For demonstration, each node's (x,y) is mapped into
 * the discrete [0..GRID_WIDTH-1, 0..GRID_HEIGHT-1] space
 * based on the bounding box of all nodes. This code can
 * be adapted to match real usage.
 */

/* A simple struct to hold integer 2D grid coordinates */
typedef struct {
    int x;
    int y;
} GridPos;

/* Node for A* search */
typedef struct {
    int   x;
    int   y;
    float f_cost;
    float g_cost;
    int   parent_x;
    int   parent_y;
    int   in_open_set;
    int   in_closed_set;
} AStarNode;

/* ---------------------------------------------------------
   Helpers to get node float positions from context
   --------------------------------------------------------- */

static int get_node_position_by_id(const AntNetContext* ctx, int node_id, float* out_x, float* out_y)
{
    if (!ctx->nodes) {
        return -1;
    }
    /* Searching for the node_id in ctx->nodes. For large data, a direct array
       indexed by node_id may be faster. This is a simple approach. */
    for (int i = 0; i < ctx->num_nodes; i++) {
        if (ctx->nodes[i].node_id == node_id) {
            *out_x = ctx->nodes[i].x;
            *out_y = ctx->nodes[i].y;
            return 0;
        }
    }
    return -1;
}

/* ---------------------------------------------------------
   Compute bounding box of the entire node set
   --------------------------------------------------------- */
static void compute_bounding_box(const AntNetContext* ctx, float* min_x, float* min_y, float* max_x, float* max_y)
{
    *min_x =  1e9f;
    *min_y =  1e9f;
    *max_x = -1e9f;
    *max_y = -1e9f;

    for (int i = 0; i < ctx->num_nodes; i++) {
        float nx = ctx->nodes[i].x;
        float ny = ctx->nodes[i].y;
        if (nx < *min_x) *min_x = nx;
        if (ny < *min_y) *min_y = ny;
        if (nx > *max_x) *max_x = nx;
        if (ny > *max_y) *max_y = ny;
    }
    /* Avoid degenerate zero-area bounding box */
    if ((*max_x - *min_x) < 1.0f) {
        *max_x = *min_x + 1.0f;
    }
    if ((*max_y - *min_y) < 1.0f) {
        *max_y = *min_y + 1.0f;
    }
}

/* ---------------------------------------------------------
   Convert a node's position (x,y) into discrete grid coords
   --------------------------------------------------------- */
static void map_to_grid(
    float x, float y,
    float min_x, float min_y,
    float max_x, float max_y,
    int *gx, int *gy
)
{
    float rx = (x - min_x) / (max_x - min_x);
    float ry = (y - min_y) / (max_y - min_y);

    /* clamp for safety */
    if (rx < 0.0f) rx = 0.0f;
    if (rx > 1.0f) rx = 1.0f;
    if (ry < 0.0f) ry = 0.0f;
    if (ry > 1.0f) ry = 1.0f;

    *gx = (int)(rx * (GRID_WIDTH  - 1));
    *gy = (int)(ry * (GRID_HEIGHT - 1));
}

/* ---------------------------------------------------------
   A* logic
   --------------------------------------------------------- */

/* Accessor for A* node in the 1D array */
static inline AStarNode* get_astar_node(AStarNode* arr, int width, int x, int y)
{
    return &arr[y * width + x];
}

static float heuristic_cost(int x1, int y1, int x2, int y2)
{
    /* Manhattan distance for 4-direction moves */
    return (float)(abs(x1 - x2) + abs(y1 - y2));
}

static int pop_lowest_f_cost(AStarNode* nodes, int width, int height)
{
    float min_f = (float)INT_MAX;
    int   index = -1;
    int   total = width * height;
    for (int i = 0; i < total; i++) {
        if (nodes[i].in_open_set && nodes[i].f_cost < min_f) {
            min_f = nodes[i].f_cost;
            index = i;
        }
    }
    return index;
}

/*
 * A* search in a discrete grid with obstacles[] = 1 for blocked, 0 for free
 * pathOut: array of GridPos to store path cells
 */
static int astar_find_path(
    const int* obstacles,
    int width,
    int height,
    int start_x, int start_y,
    int goal_x,  int goal_y,
    GridPos* pathOut,
    int maxPath,
    int* outCount
)
{
    if (start_x < 0 || start_x >= width)  return 1;
    if (start_y < 0 || start_y >= height) return 1;
    if (goal_x  < 0 || goal_x  >= width)  return 1;
    if (goal_y  < 0 || goal_y  >= height) return 1;

    /* allocate nodes */
    int total = width * height;
    AStarNode* nodes = (AStarNode*)calloc((size_t)total, sizeof(AStarNode));
    if (!nodes) return 2;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            AStarNode* n = get_astar_node(nodes, width, x, y);
            n->x = x;
            n->y = y;
            n->f_cost = (float)INT_MAX;
            n->g_cost = (float)INT_MAX;
            n->parent_x = -1;
            n->parent_y = -1;
            n->in_open_set = 0;
            n->in_closed_set = 0;
        }
    }

    /* start node */
    AStarNode* startN = get_astar_node(nodes, width, start_x, start_y);
    startN->g_cost = 0.0f;
    startN->f_cost = heuristic_cost(start_x, start_y, goal_x, goal_y);
    startN->in_open_set = 1;

    int found = 0;
    int dirs[4][2] = {{0,-1},{1,0},{0,1},{-1,0}};

    while (1) {
        int curIdx = pop_lowest_f_cost(nodes, width, height);
        if (curIdx < 0) {
            /* no open set => no path */
            break;
        }
        AStarNode* current = &nodes[curIdx];
        if (current->x == goal_x && current->y == goal_y) {
            found = 1;
            break;
        }
        current->in_open_set = 0;
        current->in_closed_set = 1;

        for (int i = 0; i < 4; i++) {
            int nx = current->x + dirs[i][0];
            int ny = current->y + dirs[i][1];
            if (nx < 0 || nx >= width || ny < 0 || ny >= height) {
                continue;
            }
            if (obstacles[ny * width + nx] == 1) {
                continue;
            }
            AStarNode* neighbor = get_astar_node(nodes, width, nx, ny);
            if (neighbor->in_closed_set) {
                continue;
            }
            float tentative_g = current->g_cost + 1.0f;
            if (!neighbor->in_open_set) {
                neighbor->in_open_set = 1;
            } else if (tentative_g >= neighbor->g_cost) {
                continue;
            }
            neighbor->parent_x = current->x;
            neighbor->parent_y = current->y;
            neighbor->g_cost = tentative_g;
            neighbor->f_cost = neighbor->g_cost + heuristic_cost(nx, ny, goal_x, goal_y);
        }
    }

    if (!found) {
        free(nodes);
        return 3;
    }

    /* reconstruct path */
    {
        AStarNode* goalN = get_astar_node(nodes, width, goal_x, goal_y);
        int path_len = 0;
        AStarNode t = *goalN;

        while (!(t.x == start_x && t.y == start_y)) {
            if (path_len >= maxPath) {
                free(nodes);
                return 4; /* out of space */
            }
            pathOut[path_len].x = t.x;
            pathOut[path_len].y = t.y;
            path_len++;
            AStarNode* p = get_astar_node(nodes, width, t.parent_x, t.parent_y);
            t = *p;
        }
        if (path_len < maxPath) {
            pathOut[path_len].x = start_x;
            pathOut[path_len].y = start_y;
            path_len++;
        }
        /* reverse it */
        for (int i = 0; i < path_len/2; i++) {
            GridPos tmp = pathOut[i];
            pathOut[i] = pathOut[path_len-1-i];
            pathOut[path_len-1-i] = tmp;
        }
        *outCount = path_len;
    }
    free(nodes);
    return 0;
}

/* compress consecutive points into corners only */
static int compress_path(
    const GridPos* inP,
    int inCount,
    GridPos* outP,
    int maxOut
)
{
    if (inCount <= 0) return 0;
    if (inCount == 1) {
        if (maxOut >= 1) {
            outP[0] = inP[0];
            return 1;
        }
        return 0;
    }
    int outCount = 0;
    outP[outCount++] = inP[0];

    int dxOld = inP[1].x - inP[0].x;
    int dyOld = inP[1].y - inP[0].y;

    for (int i = 2; i < inCount; i++) {
        int dx = inP[i].x - inP[i-1].x;
        int dy = inP[i].y - inP[i-1].y;
        if (dx != dxOld || dy != dyOld) {
            if (outCount < maxOut) {
                outP[outCount++] = inP[i-1];
            }
        }
        dxOld = dx;
        dyOld = dy;
    }
    if (outCount < maxOut) {
        outP[outCount++] = inP[inCount-1];
    }
    return outCount;
}

static void gridcell_to_render_xy(
    int gx,
    int gy,
    float min_x,
    float min_y,
    float max_x,
    float max_y,
    float offset_x,
    float offset_y,
    float* out_x,
    float* out_y
)
{
    float rx = (float)gx / (float)(GRID_WIDTH  - 1);
    float ry = (float)gy / (float)(GRID_HEIGHT - 1);
    float fx = min_x + rx * (max_x - min_x);
    float fy = min_y + ry * (max_y - min_y);
    *out_x = fx + offset_x;
    *out_y = fy + offset_y;
}

/*
 * priv_render_path_grid
 *
 * Internal function that performs the entire path rendering,
 * assuming the caller has already locked the context.
 * This is used by pub_render_path_grid(...) and
 * antnet_render_path_grid_offline(...).
 */
int priv_render_path_grid(
    const AntNetContext* ctx,
    const int* node_ids,
    int node_count,
    float offset_x,
    float offset_y,
    float* out_coords,
    int max_coords,
    int* out_count
)
{
    if (!ctx || !node_ids || node_count < 2 || !out_coords || max_coords < 2 || !out_count) {
        return 1;
    }

    /* bounding box of all nodes (min_x, min_y, max_x, max_y) */
    float min_x, min_y, max_x, max_y;
    compute_bounding_box(ctx, &min_x, &min_y, &max_x, &max_y);

    /* obstacles array for the grid */
    int* obstacles = (int*)calloc((size_t)(GRID_WIDTH*GRID_HEIGHT), sizeof(int));
    if (!obstacles) {
        return 2;
    }

    int total_written = 0;

    /* For each segment in node_ids[i], node_ids[i+1], do an A* and compress. */
    for (int seg = 0; seg < (node_count - 1); seg++) {
        /* reset obstacles */
        memset(obstacles, 0, (size_t)(GRID_WIDTH*GRID_HEIGHT) * sizeof(int));

        /* mark all nodes except these two endpoints as obstacles */
        for (int n = 0; n < ctx->num_nodes; n++) {
            int nid = ctx->nodes[n].node_id;
            if (nid == node_ids[seg] || nid == node_ids[seg+1]) {
                continue;
            }
            float nx = ctx->nodes[n].x;
            float ny = ctx->nodes[n].y;
            int gx, gy;
            map_to_grid(nx, ny, min_x, min_y, max_x, max_y, &gx, &gy);
            if (gx >= 0 && gx < GRID_WIDTH && gy >= 0 && gy < GRID_HEIGHT) {
                obstacles[gy*GRID_WIDTH + gx] = 1;
            }
        }

        /* find start and goal grid coords */
        float sx, sy, gxF, gyF;
        if (get_node_position_by_id(ctx, node_ids[seg],   &sx,  &sy) != 0 ||
            get_node_position_by_id(ctx, node_ids[seg+1], &gxF, &gyF) != 0) {
            free(obstacles);
            return 3;
        }
        int start_gx, start_gy;
        map_to_grid(sx, sy, min_x, min_y, max_x, max_y, &start_gx, &start_gy);

        int goal_gx, goal_gy;
        map_to_grid(gxF, gyF, min_x, min_y, max_x, max_y, &goal_gx, &goal_gy);

        /* run A* */
        GridPos* pathCells = (GridPos*)malloc(sizeof(GridPos) * GRID_WIDTH*GRID_HEIGHT);
        if (!pathCells) {
            free(obstacles);
            return 4;
        }
        int foundCount = 0;
        int retA = astar_find_path(
            obstacles, GRID_WIDTH, GRID_HEIGHT,
            start_gx, start_gy,
            goal_gx,  goal_gy,
            pathCells,
            GRID_WIDTH*GRID_HEIGHT,
            &foundCount
        );
        if (retA != 0) {
            free(pathCells);
            free(obstacles);
            return 5;
        }

        /* compress path */
        GridPos* compressed = (GridPos*)malloc(sizeof(GridPos)*foundCount);
        if (!compressed) {
            free(pathCells);
            free(obstacles);
            return 6;
        }
        int cCount = compress_path(pathCells, foundCount, compressed, foundCount);

        /* convert to float coords */
        for (int c = 0; c < cCount; c++) {
            if ((total_written + 2) > max_coords) {
                free(compressed);
                free(pathCells);
                free(obstacles);
                return 7;
            }
            float fx, fy;
            gridcell_to_render_xy(
                compressed[c].x,
                compressed[c].y,
                min_x, min_y,
                max_x, max_y,
                offset_x, offset_y,
                &fx, &fy
            );
            out_coords[total_written]   = fx;
            out_coords[total_written+1] = fy;
            total_written += 2;
        }

        free(compressed);
        free(pathCells);
    }

    free(obstacles);
    *out_count = total_written;
    return 0;
}
