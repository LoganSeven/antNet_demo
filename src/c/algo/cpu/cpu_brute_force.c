/* Relative Path: src/c/algo/cpu/cpu_brute_force.c */
/*
 * Explores all possible node combinations and permutations incrementally.
 * Updates the global best brute-force path when a lower-latency route is found.
 * Implements a "one path per call" mechanism for demonstration or exact enumeration.
*/

#include "../../../../include/algo/cpu/cpu_brute_force.h"
#include "../../../../include/consts/error_codes.h"
#include <string.h>
#include <limits.h>
//#include <stdio.h>

static int next_permutation(int *array, int length) {
    int k = length - 2;
    while (k >= 0 && array[k] >= array[k + 1]) k--;

    if (k < 0) return 0;

    int l = length - 1;
    while (array[l] <= array[k]) l--;

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

// Helper function: generate next combination of indices
static int next_combination(int *comb, int k, int n) {
    int i = k - 1;
    comb[i]++;
    while (i >= 0 && comb[i] >= n - k + 1 + i) {
        i--;
        if (i >= 0) comb[i]++;
    }
    if (i < 0) return 0;
    for (i++; i < k; i++)
        comb[i] = comb[i - 1] + 1;
    return 1;
}

void brute_force_reset_state(AntNetContext* ctx) {
    if (!ctx) return;

    int start_id = 0, end_id = 1, count = 0;
    for (int i = 0; i < ctx->num_nodes; i++)
        if (i != start_id && i != end_id)
            ctx->brute_state.candidate_nodes[count++] = i;

    ctx->brute_state.candidate_count = count;
    ctx->brute_state.current_L = ctx->min_hops;
    ctx->brute_state.done = 0;
    ctx->brute_state.at_first_permutation = 1;
    ctx->brute_state.at_first_combination = 1;

    for (int i = 0; i < count; i++)
        ctx->brute_state.permutation[i] = i;

    for (int i = 0; i < ctx->max_hops; i++)
        ctx->brute_state.combination[i] = i;
}

int brute_force_search_step(
    AntNetContext* ctx,
    int start_id,
    int end_id,
    int* out_nodes,
    int max_size,
    int* out_path_len,
    int* out_total_latency
) {
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
            ctx->brute_state.at_first_combination = 1;
            continue;
        }

        if (ctx->brute_state.at_first_combination) {
            for (int i = 0; i < L; i++)
                ctx->brute_state.combination[i] = i;
            ctx->brute_state.at_first_combination = 0;
            ctx->brute_state.at_first_permutation = 1;
        }

        while (1) {
            if (ctx->brute_state.at_first_permutation) {
                for (int i = 0; i < L; i++)
                    ctx->brute_state.permutation[i] = ctx->brute_state.combination[i];
                ctx->brute_state.at_first_permutation = 0;
            } else if (!next_permutation(ctx->brute_state.permutation, L)) {
                ctx->brute_state.at_first_permutation = 1;
                break; // Go to next combination
            }

            int path_length = L + 2;
            if (path_length > max_size || path_length > 1024)
                return ERR_ARRAY_TOO_SMALL;

            int temp_path[1024];
            temp_path[0] = start_id;
            for (int i = 0; i < L; i++)
                temp_path[i + 1] = ctx->brute_state.candidate_nodes[ctx->brute_state.permutation[i]];
            temp_path[path_length - 1] = end_id;

            int latency_sum = 0;
            for (int p = 0; p < path_length; p++)
                latency_sum += ctx->nodes[temp_path[p]].delay_ms;

            if (ctx->brute_best_length == 0 || latency_sum < ctx->brute_best_latency) {
                ctx->brute_best_length = path_length;
                ctx->brute_best_latency = latency_sum;
                memcpy(ctx->brute_best_nodes, temp_path, sizeof(int) * path_length);
                //printf("[DEBUG][BF] 🎯 New best path! Latency improved: %d\n", latency_sum);
            }

            goto COPY_BEST_AND_EXIT; // TEST ONE PATH PER CALL!
        }

        if (!next_combination(ctx->brute_state.combination, L, candidate_count)) {
            ctx->brute_state.current_L++;
            ctx->brute_state.at_first_combination = 1;
        }
    }

    ctx->brute_state.done = 1;

COPY_BEST_AND_EXIT:
    memcpy(out_nodes, ctx->brute_best_nodes, ctx->brute_best_length * sizeof(int));
    *out_path_len = ctx->brute_best_length;
    *out_total_latency = ctx->brute_best_latency;

    //printf("[DEBUG][BF] ✅ Best path returned: latency=%d, path=[", ctx->brute_best_latency);
    //for (int i = 0; i < ctx->brute_best_length; i++)
    //    printf("%d%s", ctx->brute_best_nodes[i], i < ctx->brute_best_length - 1 ? ", " : "");
    //printf("]\n");

    return ERR_SUCCESS;
}
