/* Relative Path: src/c/algo/cpu/cpu_ACOv1_shared_structs.c */
/*
 * Defines and handles thread-local structures for pheromone deltas and best paths.
 * Provides safe creation, merging, and destruction of these auxiliary data objects.
 * Helps avoid concurrency issues in multi-threaded ACO iterations.
*/


#include "../../../../include/algo/cpu/cpu_ACOv1_shared_structs.h"
#include "../../../../include/consts/error_codes.h"
#include <stdlib.h>
#include <string.h>

/*
 * aco_shared_create_local_data
 * Allocates an array of float for the pheromone deltas plus best-path placeholders.
 */
AcoThreadLocalData *aco_shared_create_local_data(int pheromone_size)
{
    if (pheromone_size <= 0) {
        return NULL;
    }

    AcoThreadLocalData *data = (AcoThreadLocalData *)malloc(sizeof(AcoThreadLocalData));
    if (!data) {
        return NULL;
    }
    memset(data, 0, sizeof(AcoThreadLocalData));

    /* allocate local delta pheromone array (size = n*n) */
    int total = pheromone_size * pheromone_size;
    data->delta_pheromones = (float *)malloc(sizeof(float) * total);
    if (!data->delta_pheromones) {
        free(data);
        return NULL;
    }
    memset(data->delta_pheromones, 0, sizeof(float) * total);

    data->best_length = 0;
    data->best_latency = 0;

    return data;
}

/*
 * aco_shared_free_local_data
 * Releases any allocated memory for the AcoThreadLocalData structure.
 */
void aco_shared_free_local_data(AcoThreadLocalData *data)
{
    if (!data) return;
    if (data->delta_pheromones) {
        free(data->delta_pheromones);
        data->delta_pheromones = NULL;
    }
    free(data);
}

/*
 * aco_shared_merge_deltas
 * Sums each thread's delta_pheromones into the global ctx->aco_v1.pheromones.
 * Also updates global best path if the thread's path is better.
 * Thread-safe with a single lock for the entire merge process.
 */
int aco_shared_merge_deltas(AntNetContext *ctx, AcoThreadLocalData **thread_locals, int count)
{
    if (!ctx || !thread_locals || count <= 0) {
        return ERR_INVALID_ARGS;
    }

#ifndef _WIN32
    pthread_mutex_lock(&ctx->lock);
#endif

    /* verify basic environment */
    if (!ctx->aco_v1.pheromones || ctx->aco_v1.pheromone_size <= 0) {
#ifndef _WIN32
        pthread_mutex_unlock(&ctx->lock);
#endif
        return ERR_NO_TOPOLOGY;
    }

    int n = ctx->aco_v1.pheromone_size;
    int total = n * n;

    /* accumulate deltas */
    for (int i = 0; i < count; i++) {
        AcoThreadLocalData *tlocal = thread_locals[i];
        if (!tlocal) continue;

        for (int j = 0; j < total; j++) {
            ctx->aco_v1.pheromones[j] += tlocal->delta_pheromones[j];
            if (ctx->aco_v1.pheromones[j] < 1e-6f) {
                ctx->aco_v1.pheromones[j] = 1e-6f;
            }
        }

        /* check if the thread found a better path */
        if (tlocal->best_length > 0) {
            int lat = tlocal->best_latency;
            if (ctx->aco_best_length == 0 || lat < ctx->aco_best_latency) {
                ctx->aco_best_length = tlocal->best_length;
                ctx->aco_best_latency = lat;
                memset(ctx->aco_best_nodes, 0, sizeof(ctx->aco_best_nodes));
                memcpy(ctx->aco_best_nodes, tlocal->best_path, sizeof(int) * tlocal->best_length);
            }
        }
    }

#ifndef _WIN32
    pthread_mutex_unlock(&ctx->lock);
#endif

    return ERR_SUCCESS;
}
