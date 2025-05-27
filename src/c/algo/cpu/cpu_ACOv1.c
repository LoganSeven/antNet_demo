/* Relative Path: src/c/algo/cpu/cpu_ACOv1.c */
/*
 * Main ACO V1 algorithm combining single-ant and multi-ant modes.
 * Selects a subset of nodes via pheromone-weighted picks, updating global best path if improved.
 * Central entry point for ACO initialization, iteration, and best-path retrieval.
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include "../../../../include/consts/error_codes.h"
#include "../../../../include/algo/cpu/cpu_ACOv1.h"
#include "../../../../include/algo/cpu/cpu_ACOv1_threaded.h"
#include "../../../../include/types/antnet_aco_v1_types.h"
#include "../../../../include/rendering/heatmap_renderer_api.h"
/* Added header for path reordering */
#include "../../../../include/algo/cpu/cpu_ACOv1_path_reorder.h"

/* Single-process RNG seeding guard, similar to random_algo.c */
static int g_aco_seeded = 0;

/*
 * Forward declaration for the original single-ant approach
 * so that aco_v1_run_iteration can decide which path to take.
 */
static int aco_v1_run_iteration_single(AntNetContext* ctx);

/*
 * aco_v1_init: remains the same as before (or minimal update).
 * For demonstration, ensures it's not interfering with path selection logic here.
 */
int aco_v1_init(AntNetContext* ctx)
{
    if (!ctx) {
        //printf("[DEBUG][ACO] aco_v1_init: ctx is NULL\n");
        return ERR_INVALID_ARGS;
    }
    if (ctx->num_nodes <= 0) {
        //printf("[DEBUG][ACO] aco_v1_init: No topology or zero nodes\n");
        return ERR_NO_TOPOLOGY;
    }

    if (ctx->aco_v1.is_initialized) {
        //printf("[DEBUG][ACO] aco_v1_init: Re-initializing; freeing old data...\n");
        if (ctx->aco_v1.adjacency) {
            free(ctx->aco_v1.adjacency);
            ctx->aco_v1.adjacency = NULL;
        }
        if (ctx->aco_v1.pheromones) {
            free(ctx->aco_v1.pheromones);
            ctx->aco_v1.pheromones = NULL;
        }
        ctx->aco_v1.is_initialized = 0;
    }

    int n = ctx->num_nodes;
    ctx->aco_v1.adjacency_size = n;
    ctx->aco_v1.pheromone_size = n;

    size_t matrix_count = (size_t)n * (size_t)n;

    ctx->aco_v1.adjacency = (int*)calloc(matrix_count, sizeof(int));
    if (!ctx->aco_v1.adjacency) {
        //printf("[DEBUG][ACO] aco_v1_init: adjacency allocation failed\n");
        return ERR_MEMORY_ALLOCATION;
    }

    ctx->aco_v1.pheromones = (float*)calloc(matrix_count, sizeof(float));
    if (!ctx->aco_v1.pheromones) {
        //printf("[DEBUG][ACO] aco_v1_init: pheromones allocation failed\n");
        free(ctx->aco_v1.adjacency);
        ctx->aco_v1.adjacency = NULL;
        return ERR_MEMORY_ALLOCATION;
    }

    /* Build adjacency from edges, undirected assumption */
    for (int e = 0; e < ctx->num_edges; e++) {
        int from = ctx->edges[e].from_id;
        int to   = ctx->edges[e].to_id;
        if (from >= 0 && from < n && to >= 0 && to < n) {
            ctx->aco_v1.adjacency[from * n + to] = 1;
            ctx->aco_v1.adjacency[to   * n + from] = 1;
        }
    }

    /* Initialize all pheromones to 1.0f by default */
    for (size_t i = 0; i < matrix_count; i++) {
        ctx->aco_v1.pheromones[i] = 1.0f;
    }

    /* Default parameters if not set externally */
    ctx->aco_v1.alpha       = 1.0f;
    ctx->aco_v1.beta        = 2.0f;
    ctx->aco_v1.evaporation = 0.1f;
    ctx->aco_v1.Q           = 500.0f;
    /* 
     * The original code forced 'num_ants = 1' for single-ant approach.
     * The new code can override this. If you want multi-ant, do:
     *    ctx->aco_v1.num_ants = 2; 
     * or more. 
     */
    if (ctx->aco_v1.num_ants <= 0) {
        ctx->aco_v1.num_ants = 1;
    }

    if (!g_aco_seeded) {
        g_aco_seeded = 1;
        srand((unsigned int)time(NULL));
    }

    ctx->aco_v1.is_initialized = 1;
    ctx->aco_best_length  = 0;
    ctx->aco_best_latency = 0;

    //printf("[ACO] aco_v1_init done: node_count=%d, edges=%d\n",
    //       ctx->num_nodes, ctx->num_edges);
    return ERR_SUCCESS;
}

/*
 * aco_v1_run_iteration:
 *   Now decides at runtime if we do single-ant or multi-ant approach,
 *   based on ctx->aco_v1.num_ants.
 */
int aco_v1_run_iteration(AntNetContext* ctx)
{
    if (!ctx) {
        //printf("[DEBUG][ACO] aco_v1_run_iteration: ctx=NULL\n");
        return ERR_INVALID_ARGS;
    }
    if (!ctx->aco_v1.is_initialized) {
        int rc = aco_v1_init(ctx);
        if (rc != ERR_SUCCESS) {
            return rc;
        }
    }
    if (ctx->num_nodes <= 0 || !ctx->nodes) {
        return ERR_NO_TOPOLOGY;
    }

    /* If multiple ants, run the threaded approach. */
    if (ctx->aco_v1.num_ants > 1) {
        return aco_v1_run_iteration_threaded(ctx);
    } else {
        /* Otherwise run the original single-ant approach. */
        return aco_v1_run_iteration_single(ctx);
    }
}

/*
 * aco_v1_run_iteration_single:
 *   The original single-ant approach unchanged.
 */
static int aco_v1_run_iteration_single(AntNetContext* ctx)
{
    /* min_hops..max_hops logic, same as random. */
    int range_size = ctx->max_hops - ctx->min_hops + 1;
    if (range_size <= 0) {
        //printf("[DEBUG][ACO] aco_v1_run_iteration_single: invalid hop range\n");
        return ERR_INVALID_ARGS;
    }
    int candidate_count = ctx->num_nodes - 2; /* exclude node 0 & 1 from the subset */
    if (candidate_count < 0) {
        //printf("[DEBUG][ACO] aco_v1_run_iteration_single: not enough nodes besides [0,1].\n");
        return ERR_NO_PATH_FOUND;
    }

    int nb_selected_nodes = ctx->min_hops + (rand() % range_size);
    if (nb_selected_nodes > candidate_count) {
        nb_selected_nodes = candidate_count;
    }

    /* Build a local array of possible nodes = [2..n-1]. We weight them by node-level pheromone. */
    int* node_list = (int*)malloc((size_t)candidate_count * sizeof(int));
    if (!node_list) {
        return ERR_MEMORY_ALLOCATION;
    }
    int idx = 0;
    for (int i = 0; i < ctx->num_nodes; i++) {
        if (i != 0 && i != 1) {
            node_list[idx++] = i;
        }
    }

    /* Build an array node_weight[i_in_node_list], computed as sum of pheromones[i*n + k] for k in [0..n-1]. */
    float* node_weight = (float*)malloc((size_t)candidate_count * sizeof(float));
    if (!node_weight) {
        free(node_list);
        return ERR_MEMORY_ALLOCATION;
    }

    int n = ctx->aco_v1.pheromone_size;
    float total_weight = 0.0f;

    for (int c = 0; c < candidate_count; c++) {
        int node_id = node_list[c];
        float sum_pher = 0.0f;
        for (int k = 0; k < n; k++) {
            sum_pher += ctx->aco_v1.pheromones[node_id * n + k];
        }
        if (sum_pher < 1e-6f) {
            sum_pher = 1e-6f; /* avoid zero or negative */
        }
        node_weight[c] = sum_pher;
        total_weight   += sum_pher;
    }

    /* Pick nb_selected_nodes by a "weighted draw without replacement" approach. */
    int* chosen_nodes = (int*)malloc((size_t)nb_selected_nodes * sizeof(int));
    if (!chosen_nodes) {
        free(node_weight);
        free(node_list);
        return ERR_MEMORY_ALLOCATION;
    }

    int chosen_count = 0;
    int remain = candidate_count;

    for (int pick = 0; pick < nb_selected_nodes; pick++) {
        if (remain <= 0 || total_weight <= 1e-9f) {
            break;
        }
        float r = (float)rand() / (float)RAND_MAX;
        float accum = 0.0f;
        int chosen_index = -1;
        for (int c = 0; c < remain; c++) {
            float p = node_weight[c] / total_weight;
            accum += p;
            if (r <= accum) {
                chosen_index = c;
                break;
            }
        }
        if (chosen_index < 0) {
            chosen_index = remain - 1;
        }

        chosen_nodes[chosen_count++] = node_list[chosen_index];

        /* remove from arrays: swap last -> chosen_index */
        total_weight -= node_weight[chosen_index];
        node_list[chosen_index]   = node_list[remain - 1];
        node_weight[chosen_index] = node_weight[remain - 1];
        remain--;
    }

    /* If not as many picked as intended, it proceeds anyway (like random). */

    /* Fisher-Yates shuffle to replicate random's final order. */
    for (int i = chosen_count - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = chosen_nodes[i];
        chosen_nodes[i] = chosen_nodes[j];
        chosen_nodes[j] = tmp;
    }

    free(node_weight);
    free(node_list);

    /* Build final path [0, chosen_nodes..., 1]. */
    int new_path_length = chosen_count + 2; /* start + subset + end */
    int* new_path = (int*)malloc((size_t)new_path_length * sizeof(int));
    if (!new_path) {
        free(chosen_nodes);
        return ERR_MEMORY_ALLOCATION;
    }
    new_path[0] = 0;
    for (int i = 0; i < chosen_count; i++) {
        new_path[i + 1] = chosen_nodes[i];
    }
    new_path[new_path_length - 1] = 1;

    free(chosen_nodes);

    /* Sum cost, check for overflow */
    int cost_sum = 0;
    for (int k = 0; k < new_path_length; k++) {
        int node_id = new_path[k];
        if (node_id < 0 || node_id >= ctx->num_nodes) {
            free(new_path);
            return ERR_NO_PATH_FOUND;
        }
        if (ctx->nodes[node_id].delay_ms > INT_MAX - cost_sum) {
            free(new_path);
            return ERR_INVALID_ARGS;
        }
        cost_sum += ctx->nodes[node_id].delay_ms;
    }

    /* If better or if none yet, store in ctx->aco_best_* */
    if (ctx->aco_best_length == 0 || cost_sum < ctx->aco_best_latency) {
        ctx->aco_best_length = new_path_length;
        ctx->aco_best_latency = cost_sum;
        for (int p = 0; p < new_path_length; p++) {
            ctx->aco_best_nodes[p] = new_path[p];
        }
    }

    /* Evaporate and reinforce pheromones for the edges in the new path */
    for (int i = 0; i < new_path_length - 1; i++) {
        int from = new_path[i];
        int to   = new_path[i + 1];
        int idx  = from * n + to;

        ctx->aco_v1.pheromones[idx] *= (1.0f - ctx->aco_v1.evaporation);
        ctx->aco_v1.pheromones[idx] += ctx->aco_v1.Q / (float)cost_sum;
        if (ctx->aco_v1.pheromones[idx] < 1e-6f) {
            ctx->aco_v1.pheromones[idx] = 1e-6f;
        }
    }

    //printf("[DEBUG][ACO] Reinforced %d-hop path, cost=%d\n", new_path_length - 2, cost_sum);

    free(new_path);

    return ERR_SUCCESS;
}

/*
 * aco_v1_get_best_path: same as typical; copies best to out_* fields.
 * Now calls aco_v1_reorder_path_for_display to reorder the intermediate nodes
 * between [0..1] prior to returning, purely for external/visual usage.
 */
int aco_v1_get_best_path(
    AntNetContext* ctx,
    int* out_nodes,
    int max_size,
    int* out_path_len,
    int* out_total_latency
)
{
    if (!ctx || !out_nodes || !out_path_len || !out_total_latency) {
        return ERR_INVALID_ARGS;
    }
    if (ctx->aco_best_length == 0) {
        return ERR_NO_PATH_FOUND;
    }
    if (ctx->aco_best_length > max_size) {
        return ERR_ARRAY_TOO_SMALL;
    }

    memcpy(out_nodes, ctx->aco_best_nodes, ctx->aco_best_length * sizeof(int));
    *out_path_len = ctx->aco_best_length;
    *out_total_latency = ctx->aco_best_latency;

    /* Reorder only for display, does not modify the stored best path in ctx->aco_best_nodes. */
    aco_v1_reorder_path_for_display(out_nodes, *out_path_len);

    return ERR_SUCCESS;
}

/*
 * aco_v1_search_path:
 *   Just calls aco_v1_get_best_path. No ants, no extra logic,
 *   preserving the function signature. Matches how random_algo does get_best_path.
 */
int aco_v1_search_path(
    AntNetContext* ctx,
    int start_id,
    int end_id,
    int* out_nodes,
    int max_size,
    int* out_path_len,
    int* out_total_latency
)
{
    (void)start_id; /* unused in this approach, same as random */
    (void)end_id;

    int rc = aco_v1_get_best_path(ctx, out_nodes, max_size, out_path_len, out_total_latency);
    return rc;
}
