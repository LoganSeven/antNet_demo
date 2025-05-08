// src/c/cpu_brute_force.c
#include "../../include/cpu_brute_force.h"
#include "../../include/antnet_brute_force_types.h"
#include "../../include/error_codes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

/*
 * next_permutation: generates the next lexicographical permutation
 * of array[] in place. Returns 1 if the permutation was advanced,
 * or 0 if array[] was the last permutation (no next permutation).
 */
static int next_permutation(int *array, int length)
{
    int k = length - 2;
    while (k >= 0 && array[k] >= array[k + 1]) {
        k--;
    }
    if (k < 0) {
        return 0; /* no next permutation */
    }
    int l = length - 1;
    while (array[l] <= array[k]) {
        l--;
    }
    /* swap array[k], array[l] */
    {
        int tmp = array[k];
        array[k] = array[l];
        array[l] = tmp;
    }
    /* reverse the tail from k+1 to end */
    for (int i = k + 1, j = length - 1; i < j; i++, j--) {
        int tmp = array[i];
        array[i] = array[j];
        array[j] = tmp;
    }
    return 1;
}

/*
 * brute_force_reset_state: restarts iteration from min_hops, sets at_first_permutation=1,
 * but keeps the best path found so far in ctx->brute_best_*.
 *
 * IMPORTANT: The caller is expected to hold ctx->lock (if needed),
 *            so we do not lock here to avoid double-locking.
 */
void brute_force_reset_state(AntNetContext* ctx)
{
    if (!ctx) {
        return;
    }

    /* security: re-build candidate_nodes each time in case topology changed */
    int start_id = 0;
    int end_id   = 1;

    /* Rebuild candidate_nodes array from available nodes, ignoring start/end */
    int candidate_count = 0;
    if (ctx->nodes && ctx->num_nodes > 2) {
        for (int i = 0; i < ctx->num_nodes; i++) {
            if (i != start_id && i != end_id) {
                ctx->brute_state.candidate_nodes[candidate_count++] = i;
            }
        }
    }
    ctx->brute_state.candidate_count = candidate_count;

    /*
     * We begin at min_hops. The solver enumerates path lengths from min_hops..max_hops.
     */
    ctx->brute_state.current_L = ctx->min_hops;
    ctx->brute_state.at_first_permutation = 1;
    ctx->brute_state.done = 0;
}

/*
 * brute_force_search_step: enumerates exactly one path from the solver,
 * updates the best path if better, copies the best so far into out_*.
 * Returns 0 on success, negative on error.
 */
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
    if (!ctx || !out_nodes || !out_path_len || !out_total_latency) {
        return ERR_INVALID_ARGS;
    }

#ifndef _WIN32
    pthread_mutex_lock(&ctx->lock);
#endif

    if (ctx->num_nodes <= 0 || !ctx->nodes) {
#ifndef _WIN32
        pthread_mutex_unlock(&ctx->lock);
#endif
        return ERR_NO_TOPOLOGY;
    }

    if (ctx->brute_state.done) {
        /* Solver has already enumerated all possibilities; just output best path. */
        goto COPY_BEST_AND_EXIT;
    }

    /* security: check array capacity (worst case path = max_hops + 2) */
    if (ctx->max_hops + 2 > max_size || ctx->max_hops + 2 > 1024) {
#ifndef _WIN32
        pthread_mutex_unlock(&ctx->lock);
#endif
        return ERR_ARRAY_TOO_SMALL;
    }

    /* If candidate_count < current_L, skip to the next L until we find feasible or exhaust. */
    while (ctx->brute_state.current_L <= ctx->max_hops) {
        int L = ctx->brute_state.current_L;
        int candidate_count = ctx->brute_state.candidate_count;

        if (L > candidate_count) {
            ctx->brute_state.current_L++;
            ctx->brute_state.at_first_permutation = 1;
            continue;
        }

        if (ctx->brute_state.at_first_permutation) {
            /* initialize permutation in ascending order [0..candidate_count-1] */
            for (int i = 0; i < candidate_count; i++) {
                ctx->brute_state.permutation[i] = i;
            }
            ctx->brute_state.at_first_permutation = 0;
        } else {
            /* produce the next permutation, if any */
            if (!next_permutation(ctx->brute_state.permutation, candidate_count)) {
                /* all permutations at this length exhausted → next length */
                ctx->brute_state.current_L++;
                ctx->brute_state.at_first_permutation = 1;
                continue;
            }
        }

        /* Build exactly one new path using the first L items in permutation[] */
        {
            int new_path_length = L + 2; /* start + L intermediates + end */
            int* new_path = (int*)malloc(sizeof(int) * (size_t)new_path_length);
            if (!new_path) {
#ifndef _WIN32
                pthread_mutex_unlock(&ctx->lock);
#endif
                return ERR_MEMORY_ALLOCATION;
            }

            new_path[0] = start_id;
            for (int k = 0; k < L; k++) {
                int idx = ctx->brute_state.permutation[k];
                new_path[k + 1] = ctx->brute_state.candidate_nodes[idx];
            }
            new_path[new_path_length - 1] = end_id;

            /* Compute total latency */
            int new_total_latency = 0;
            for (int p = 0; p < new_path_length; p++) {
                int node_id = new_path[p];
                if (node_id < 0 || node_id >= ctx->num_nodes) {
                    free(new_path);
#ifndef _WIN32
                    pthread_mutex_unlock(&ctx->lock);
#endif
                    return ERR_NO_PATH_FOUND;
                }
                /* security: check overflow */
                if (ctx->nodes[node_id].delay_ms > INT_MAX - new_total_latency) {
                    free(new_path);
#ifndef _WIN32
                    pthread_mutex_unlock(&ctx->lock);
#endif
                    return ERR_INVALID_ARGS;
                }
                new_total_latency += ctx->nodes[node_id].delay_ms;
            }

            /* If better or none stored yet, update best. */
            if (ctx->brute_best_length == 0 || new_total_latency < ctx->brute_best_latency) {
                ctx->brute_best_length  = new_path_length;
                ctx->brute_best_latency = new_total_latency;
                for (int p = 0; p < new_path_length; p++) {
                    ctx->brute_best_nodes[p] = new_path[p];
                }
            }

            free(new_path);

            /* Found exactly 1 path in this step → go copy best path & exit. */
            goto COPY_BEST_AND_EXIT;
        }
    }

    /* If we got here, we've exhausted all L up to max_hops → done. */
    ctx->brute_state.done = 1;

COPY_BEST_AND_EXIT:
    /* Return the best path so far. */
    if (ctx->brute_best_length > max_size) {
#ifndef _WIN32
        pthread_mutex_unlock(&ctx->lock);
#endif
        return ERR_ARRAY_TOO_SMALL;
    }

    if (ctx->brute_best_length > 0) {
        for (int i = 0; i < ctx->brute_best_length; i++) {
            out_nodes[i] = ctx->brute_best_nodes[i];
        }
        *out_path_len      = ctx->brute_best_length;
        *out_total_latency = ctx->brute_best_latency;
    } else {
        *out_path_len      = 0;
        *out_total_latency = 0;
    }

#ifndef _WIN32
    pthread_mutex_unlock(&ctx->lock);
#endif
    return ERR_SUCCESS;
}
