// src/c/cpu_brute_force.c

#include "../../include/cpu_brute_force.h"
#include "../../include/error_codes.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>

static int next_permutation(int *array, int length)
{
    int k = length - 2;
    while (k >= 0 && array[k] >= array[k + 1]) {
        k--;
    }
    if (k < 0) {
        return 0; /* last permutation reached */
    }

    int l = length - 1;
    while (array[l] <= array[k]) {
        l--;
    }

    int tmp = array[k];
    array[k] = array[l];
    array[l] = tmp;

    for (int i = k + 1, j = length - 1; i < j; i++, j--) {
        tmp = array[i];
        array[i] = array[j];
        array[j] = tmp;
    }

    return 1;
}

void brute_force_reset_state(AntNetContext* ctx)
{
    if (!ctx) return;

    int start_id = 0;
    int end_id = 1;
    int count = 0;
    for (int i = 0; i < ctx->num_nodes; i++) {
        if (i != start_id && i != end_id) {
            ctx->brute_state.candidate_nodes[count++] = i;
        }
    }
    ctx->brute_state.candidate_count = count;

    ctx->brute_state.current_L = ctx->min_hops;
    ctx->brute_state.done = 0;
    ctx->brute_state.at_first_permutation = 1;

    /* initialize the permutation array once */
    for (int i = 0; i < count; i++) {
        ctx->brute_state.permutation[i] = i;
    }
}

int brute_force_search_step(
    AntNetContext* ctx,
    int start_id,
    int end_id,
    int* out_nodes,
    int max_size,
    int* out_path_len,
    int* out_total_latency
)
{
    if (!ctx || !out_nodes || !out_path_len || !out_total_latency)
        return ERR_INVALID_ARGS;

    if (ctx->num_nodes <= 0 || !ctx->nodes)
        return ERR_NO_TOPOLOGY;

    if (ctx->brute_state.done)
        goto COPY_BEST_AND_EXIT;

    int candidate_count = ctx->brute_state.candidate_count;

    while (ctx->brute_state.current_L <= ctx->max_hops) {
        int L = ctx->brute_state.current_L;

        if (L > candidate_count) {
            ctx->brute_state.current_L++;
            ctx->brute_state.at_first_permutation = 1;
            continue;
        }

        if (ctx->brute_state.at_first_permutation) {
            /* Reset the permutation array only once per new L */
            for (int i = 0; i < candidate_count; i++)
                ctx->brute_state.permutation[i] = i;

            ctx->brute_state.at_first_permutation = 0;
        } else {
            if (!next_permutation(ctx->brute_state.permutation, candidate_count)) {
                ctx->brute_state.current_L++;
                ctx->brute_state.at_first_permutation = 1;
                continue;
            }
        }

        printf("[DEBUG BACKEND][BF] Path length %d, candidates %d\n", L, candidate_count);

        int path_length = L + 2;
        if (path_length > max_size || path_length > 1024)
            return ERR_ARRAY_TOO_SMALL;

        int temp_path[1024];
        temp_path[0] = start_id;
        for (int i = 0; i < L; i++)
            temp_path[i + 1] = ctx->brute_state.candidate_nodes[ctx->brute_state.permutation[i]];
        temp_path[path_length - 1] = end_id;

        int latency_sum = 0;
        for (int p = 0; p < path_length; p++) {
            int node_id = temp_path[p];
            if (node_id < 0 || node_id >= ctx->num_nodes)
                return ERR_NO_PATH_FOUND;

            if (ctx->nodes[node_id].delay_ms > INT_MAX - latency_sum)
                return ERR_INVALID_ARGS;

            latency_sum += ctx->nodes[node_id].delay_ms;
        }

        if (ctx->brute_best_length == 0 || latency_sum < ctx->brute_best_latency) {
            ctx->brute_best_length = path_length;
            ctx->brute_best_latency = latency_sum;
            memcpy(ctx->brute_best_nodes, temp_path, sizeof(int) * path_length);
        }

        goto COPY_BEST_AND_EXIT;
    }

    ctx->brute_state.done = 1;

COPY_BEST_AND_EXIT:
    if (ctx->brute_best_length > max_size)
        return ERR_ARRAY_TOO_SMALL;

    if (ctx->brute_best_length > 0) {
        memcpy(out_nodes, ctx->brute_best_nodes, ctx->brute_best_length * sizeof(int));
        *out_path_len = ctx->brute_best_length;
        *out_total_latency = ctx->brute_best_latency;
    } else {
        *out_path_len = 0;
        *out_total_latency = 0;
        return ERR_NO_PATH_FOUND;
    }

    return ERR_SUCCESS;
}
