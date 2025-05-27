/* Relative Path: src/c/algo/cpu/cpu_random_algo.c */
/*
 * Explores all possible node combinations and permutations in incremental steps.
 * Updates the global best brute-force path if a lower-latency route is found.
 * Provides a stateful "one path per call" approach for demonstration or exact optimization.
*/


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include "../../../../include/consts/error_codes.h"
#include "../../../../include/algo/cpu/cpu_random_algo.h"
#include "../../../../include/rendering/heatmap_renderer_api.h"
/* Added header to reorder path for display */
#include "../../../../include/algo/cpu/cpu_random_algo_path_reorder.h"

/* security: track if we have seeded the RNG once to avoid repeated seeding on each call */
static int g_seeded = 0;

int random_search_path(
    AntNetContext* ctx,
    int start_id,
    int end_id,
    int* out_nodes,
    int max_size,
    int* out_path_len,
    int* out_total_latency
)
{
    if (!ctx || !out_nodes || !out_path_len || !out_total_latency) {
        return ERR_INVALID_ARGS;
    }
    if (ctx->num_nodes <= 0 || ctx->nodes == NULL) {
        return ERR_NO_TOPOLOGY;
    }

    /* Ensure the result array is large enough for worst case (max_hops + 2) */
    int needed_capacity = ctx->max_hops + 2;
    if (needed_capacity > max_size || needed_capacity > 1024) {
        return ERR_ARRAY_TOO_SMALL;
    }

    /* seed once per process run */
    if (!g_seeded) {
        srand((unsigned int)time(NULL));
        g_seeded = 1;
    }

    int range_size = ctx->max_hops - ctx->min_hops + 1;
    if (range_size <= 0) {
        return ERR_INVALID_ARGS;
    }

    /* pick nb_selected_nodes in [min_hops..max_hops], clamp by candidate_count */
    int nb_selected_nodes = ctx->min_hops + (rand() % range_size);
    int candidate_count = ctx->num_nodes - 2; /* exclude start_id and end_id */

    if (candidate_count < 0) {
        return ERR_NO_PATH_FOUND; /* no valid nodes besides start/end */
    }
    if (nb_selected_nodes > candidate_count) {
        nb_selected_nodes = candidate_count;
    }

    int* candidates = (int*)malloc((size_t)candidate_count * sizeof(int));
    if (!candidates) {
        return ERR_MEMORY_ALLOCATION;
    }

    int idx = 0;
    for (int i = 0; i < ctx->num_nodes; i++) {
        if (i != start_id && i != end_id) {
            candidates[idx++] = i;
        }
    }

    int new_path_length = nb_selected_nodes + 2; /* including start, end */
    int* new_path = (int*)malloc((size_t)new_path_length * sizeof(int));
    if (!new_path) {
        free(candidates);
        return ERR_MEMORY_ALLOCATION;
    }

    /* Fisher-Yates shuffle of the candidates array */
    for (int i = candidate_count - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = candidates[i];
        candidates[i] = candidates[j];
        candidates[j] = tmp;
    }

    new_path[0] = start_id;
    for (int k = 0; k < nb_selected_nodes; k++) {
        new_path[k + 1] = candidates[k];
    }
    new_path[new_path_length - 1] = end_id;

    free(candidates);

    /* compute total latency, checking for potential overflow */
    int new_total_latency = 0;
    for (int k = 0; k < new_path_length; k++) {
        int node_id = new_path[k];
        if (node_id < 0 || node_id >= ctx->num_nodes) {
            free(new_path);
            return ERR_NO_PATH_FOUND;
        }
        if (ctx->nodes[node_id].delay_ms > INT_MAX - new_total_latency) {
            free(new_path);
            return ERR_INVALID_ARGS;
        }
        new_total_latency += ctx->nodes[node_id].delay_ms;
    }

    /*
     * if better or if none yet, update ctx->random_best_*
     */
    if (ctx->random_best_length == 0 || new_total_latency < ctx->random_best_latency) {
        ctx->random_best_length = new_path_length;
        ctx->random_best_latency = new_total_latency;
        for (int p = 0; p < new_path_length; p++) {
            ctx->random_best_nodes[p] = new_path[p];
        }
    }

    free(new_path);

    /* 
     * Now copy the best path so far to out_* fields
     * but reorder it for display: sorts the subarray [1..end-1].
     */
    if (ctx->random_best_length > max_size) {
        return ERR_ARRAY_TOO_SMALL;
    }

    /* copy best path into out_nodes */
    for (int p = 0; p < ctx->random_best_length; p++) {
        out_nodes[p] = ctx->random_best_nodes[p];
    }

    /* reorder for display (purely cosmetic, does not change stored best) */
    random_algo_reorder_path_for_display(out_nodes, ctx->random_best_length);

    *out_path_len = ctx->random_best_length;
    *out_total_latency = ctx->random_best_latency;

    return ERR_SUCCESS;
}
