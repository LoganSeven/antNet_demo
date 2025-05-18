/* Relative Path: src/c/algo/cpu/cpu_ACOv1_threaded.c */

/*
 * cpu_ACOv1_threaded.c
 * Implements the multi-threaded version of a single ACO iteration. Each ant
 * has a private local delta array to avoid race conditions. A single lock is used
 * during the final merge stage only.
 */

#include "../../../../include/algo/cpu/cpu_ACOv1_threaded.h"
#include "../../../../include/algo/cpu/cpu_ACOv1_shared_structs.h"
#include "../../../../include/algo/cpu/cpu_ACOv1.h"
#include "../../../../include/types/antnet_aco_v1_types.h"
#include "../../../../include/core/backend.h"
#include "../../../../include/consts/error_codes.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <pthread.h>
#endif
#include <limits.h>
#include <time.h>

/*
 * Internal function: aco_build_path_for_one_ant
 * Replicates the single-ant logic from aco_v1_run_iteration_single, 
 * but modifies nothing globally. Instead, it calculates the path,
 * cost, and local pheromone deltas. 
 */
static int aco_build_path_for_one_ant(AntNetContext* ctx, AcoThreadLocalData* local_data)
{
    /* Basic checks */
    if (!ctx || !local_data) {
        return ERR_INVALID_ARGS;
    }

    int range_size = ctx->max_hops - ctx->min_hops + 1;
    if (range_size <= 0) {
        return ERR_INVALID_ARGS;
    }
    int candidate_count = ctx->num_nodes - 2; /* exclude node 0 & 1 from the subset */
    if (candidate_count < 0) {
        return ERR_NO_PATH_FOUND;
    }

    int nb_selected_nodes = ctx->min_hops + (rand() % range_size);
    if (nb_selected_nodes > candidate_count) {
        nb_selected_nodes = candidate_count;
    }

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

    float* node_weight = (float*)malloc((size_t)candidate_count * sizeof(float));
    if (!node_weight) {
        free(node_list);
        return ERR_MEMORY_ALLOCATION;
    }

    int n = ctx->aco_v1.pheromone_size;
    float total_weight = 0.0f;

    /* sum pheromones for each candidate node i: sum_{k} pheromones[i*n + k] */
    for (int c = 0; c < candidate_count; c++) {
        int node_id = node_list[c];
        float sum_pher = 0.0f;
        /* Safe read from global pheromones (no writer at the same time) */
        for (int k = 0; k < n; k++) {
            sum_pher += ctx->aco_v1.pheromones[node_id * n + k];
        }
        if (sum_pher < 1e-6f) {
            sum_pher = 1e-6f;
        }
        node_weight[c] = sum_pher;
        total_weight   += sum_pher;
    }

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

        total_weight -= node_weight[chosen_index];
        node_list[chosen_index]   = node_list[remain - 1];
        node_weight[chosen_index] = node_weight[remain - 1];
        remain--;
    }

    /* shuffle */
    for (int i = chosen_count - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = chosen_nodes[i];
        chosen_nodes[i] = chosen_nodes[j];
        chosen_nodes[j] = tmp;
    }

    free(node_weight);
    free(node_list);

    int new_path_length = chosen_count + 2;
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

    /* store best path in local_data if better or if none yet */
    if (local_data->best_length == 0 || cost_sum < local_data->best_latency) {
        local_data->best_length  = new_path_length;
        local_data->best_latency = cost_sum;
        memcpy(local_data->best_path, new_path, (size_t)new_path_length * sizeof(int));
    }

    /* Prepare local pheromone deltas for edges in the new path 
     * newVal = oldVal*(1-evap) + Q/cost, so delta = newVal - oldVal = -oldVal*evap + Q/cost.
     */
    float evap = ctx->aco_v1.evaporation;
    float Q    = ctx->aco_v1.Q;

    for (int i = 0; i < new_path_length - 1; i++) {
        int from = new_path[i];
        int to   = new_path[i + 1];
        int index = from * n + to;

        float oldVal = ctx->aco_v1.pheromones[index];
        float newVal = oldVal * (1.0f - evap) + (Q / (float)cost_sum);
        float delta  = newVal - oldVal;
        local_data->delta_pheromones[index] += delta;
    }

    free(new_path);
    return ERR_SUCCESS;
}

/*
 * Internal function: aco_thread_func
 * The routine run by each ant thread. Populates local_data->delta_pheromones and best path.
 */
static void *aco_thread_func(void *arg)
{
    AcoThreadLocalData *local_data = (AcoThreadLocalData *)arg;
    if (!local_data) {
#ifndef _WIN32
        pthread_exit(NULL);
#endif
        return NULL;
    }

    /* We need a pointer to the context. In a real design, we'd pass a struct containing both context + local_data. */
    /* But for simplicity, store a static pointer in thread-local or pass via a global. 
     * We do not want to break the architecture, so let's do a minimal approach:
     * We'll assume a global pointer is set by the caller. 
     */

    /* This is not fully ideal in large code, but remains a straightforward approach for a small demonstration. */
    extern __thread AntNetContext *g_aco_shared_ctx_tls; /* thread-local storage in POSIX style */
    /* fallback if no __thread support, we do a static global. This is a known limitation. */
    if (!g_aco_shared_ctx_tls) {
#ifndef _WIN32
        pthread_exit(NULL);
#endif
        return NULL;
    }

    aco_build_path_for_one_ant(g_aco_shared_ctx_tls, local_data);

#ifndef _WIN32
    pthread_exit(NULL);
#endif
    return NULL;
}

/* 
 * g_aco_shared_ctx_tls
 * A thread-local pointer to the current context. 
 */
#ifndef _WIN32
__thread AntNetContext *g_aco_shared_ctx_tls = NULL;
#else
/* On Windows, we do a simple global fallback for demonstration. */
static AntNetContext *g_aco_shared_ctx_tls = NULL;
#endif

/*
 * aco_v1_run_iteration_threaded
 * Spawns ctx->aco_v1.num_ants threads, each performing one "ant" iteration with local deltas.
 * Then merges the results under a lock.
 */
int aco_v1_run_iteration_threaded(AntNetContext *ctx)
{
    if (!ctx) return ERR_INVALID_CONTEXT;
    if (ctx->aco_v1.pheromone_size <= 0) return ERR_NO_TOPOLOGY;
    if (ctx->aco_v1.num_ants <= 1) {
        /* fallback to single approach if misused */
        return aco_v1_run_iteration(ctx);
    }

    /* set the global pointer for all threads to see (not elegant but minimal) */
    g_aco_shared_ctx_tls = ctx;

    int ants = ctx->aco_v1.num_ants;

    AcoThreadLocalData **thread_data = (AcoThreadLocalData **)malloc((size_t)ants * sizeof(AcoThreadLocalData*));
    if (!thread_data) {
        return ERR_MEMORY_ALLOCATION;
    }
    memset(thread_data, 0, (size_t)ants * sizeof(AcoThreadLocalData*));

#ifndef _WIN32
    pthread_t *threads = (pthread_t *)malloc((size_t)ants * sizeof(pthread_t));
    if (!threads) {
        free(thread_data);
        return ERR_MEMORY_ALLOCATION;
    }
#endif

    /* allocate local data for each ant */
    for (int i = 0; i < ants; i++) {
        thread_data[i] = aco_shared_create_local_data(ctx->aco_v1.pheromone_size);
        if (!thread_data[i]) {
            /* cleanup partial allocations if any fail */
            for (int j = 0; j < i; j++) {
                aco_shared_free_local_data(thread_data[j]);
                thread_data[j] = NULL;
            }
#ifndef _WIN32
            free(threads);
#endif
            free(thread_data);
            return ERR_MEMORY_ALLOCATION;
        }
    }

    /* launch each ant in its own thread */
#ifndef _WIN32
    for (int i = 0; i < ants; i++) {
        pthread_create(&threads[i], NULL, aco_thread_func, thread_data[i]);
    }

    /* wait for all threads to finish */
    for (int i = 0; i < ants; i++) {
        pthread_join(threads[i], NULL);
    }
#else
    /* fallback single-thread loop for Windows placeholder, repeated 'ants' times */
    for (int i = 0; i < ants; i++) {
        aco_thread_func(thread_data[i]);
    }
#endif

    /* merge local deltas into the global pheromones */
    int rc = aco_shared_merge_deltas(ctx, thread_data, ants);

    /* free all local data */
    for (int i = 0; i < ants; i++) {
        aco_shared_free_local_data(thread_data[i]);
        thread_data[i] = NULL;
    }
    free(thread_data);

#ifndef _WIN32
    free(threads);
#endif

    /* clear the global pointer */
    g_aco_shared_ctx_tls = NULL;

    return rc;
}
