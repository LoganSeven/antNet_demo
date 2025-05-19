/* Relative Path: src/c/core/backend.c */
/*
 * The relevant changes are in antnet_run_all_solvers to incorporate ACO logic
 * plus the newly added antnet_get_config for reading the AppConfig.
 * The rest of the file remains the same, except for new lines with aco_v1 calls,
 * the new SASA param usage, new param setters/getters,
 * and the final function antnet_render_heatmap_rgba which now calls the async approach.
 * Two new functions at the end manage the async renderer lifecycle:
 *    antnet_renderer_async_init
 *    antnet_renderer_async_shutdown
 */

#include "../../../include/core/backend.h"
#include "../../../include/consts/error_codes.h"
#include "../../../include/algo/cpu/cpu_ACOv1.h"
#include "../../../include/algo/cpu/cpu_random_algo.h"
#include "../../../include/algo/cpu/cpu_brute_force.h"
#include "../../../include/managers/config_manager.h"
#include "../../../include/rendering/heatmap_renderer_async.h"

/* SASA addition: include the ranking logic */
#include "../../../include/core/score_evaluation.h"

/* NEW: for the RankingEntry structure */
#include "../../../include/types/antnet_ranking_types.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h> 

#ifndef _WIN32
#include <pthread.h>
#endif

#define MAX_CONTEXTS 16

static AntNetContext g_contexts[MAX_CONTEXTS];
static int g_context_in_use[MAX_CONTEXTS] = {0};

#ifndef _WIN32
static pthread_mutex_t g_contexts_lock = PTHREAD_MUTEX_INITIALIZER;
#endif

/*
 * get_context_by_id
 * Retrieves the pointer to the context if in range and in use, else returns NULL.
 */
AntNetContext* get_context_by_id(int context_id)
{
    if (context_id < 0 || context_id >= MAX_CONTEXTS) {
        return NULL;
    }
#ifndef _WIN32
    pthread_mutex_lock(&g_contexts_lock);
#endif
    int in_use = g_context_in_use[context_id];
#ifndef _WIN32
    pthread_mutex_unlock(&g_contexts_lock);
#endif

    return in_use ? &g_contexts[context_id] : NULL;
}

/*
 * antnet_initialize
 * Creates a new context if there is a free slot, initializes default fields,
 * sets up random/bf/aco states, and returns the context_id on success.
 */
int antnet_initialize(int node_count, int min_hops, int max_hops)
{
#ifndef _WIN32
    pthread_mutex_lock(&g_contexts_lock);
#endif
    for (int i = 0; i < MAX_CONTEXTS; i++) {
        if (!g_context_in_use[i]) {
            g_context_in_use[i] = 1;
#ifndef _WIN32
            pthread_mutex_unlock(&g_contexts_lock);
#endif
            AntNetContext* ctx = &g_contexts[i];
            ctx->node_count = node_count;
            ctx->min_hops   = min_hops;
            ctx->max_hops   = max_hops;
            ctx->iteration  = 0;
            ctx->nodes      = NULL;
            ctx->edges      = NULL;
            ctx->num_nodes  = 0;
            ctx->num_edges  = 0;

            ctx->random_best_length  = 0;
            ctx->random_best_latency = 0;
            memset(ctx->random_best_nodes, 0, sizeof(ctx->random_best_nodes));

            config_set_defaults(&ctx->config);
            ctx->config.set_nb_nodes = node_count;
            ctx->config.min_hops     = min_hops;
            ctx->config.max_hops     = max_hops;

            ctx->brute_best_length  = 0;
            ctx->brute_best_latency = 0;
            memset(ctx->brute_best_nodes, 0, sizeof(ctx->brute_best_nodes));
            memset(&ctx->brute_state, 0, sizeof(ctx->brute_state));

            ctx->aco_best_length  = 0;
            ctx->aco_best_latency = 0;
            memset(ctx->aco_best_nodes, 0, sizeof(ctx->aco_best_nodes));
            memset(&ctx->aco_v1, 0, sizeof(ctx->aco_v1));

#ifndef _WIN32
            pthread_mutex_init(&ctx->lock, NULL);
#endif

            /* SASA addition: initialize SASA states for ACO, Random, Brute to default */
            init_sasa_state(&ctx->aco_sasa);
            init_sasa_state(&ctx->random_sasa);
            init_sasa_state(&ctx->brute_sasa);

            /* NEW: default SASA coefficients, previously hard-coded in run_all_solvers */
            ctx->sasa_coeffs.alpha = 0.4;
            ctx->sasa_coeffs.beta  = 0.4;
            ctx->sasa_coeffs.gamma = 0.2;

            return i;
        }
    }
#ifndef _WIN32
    pthread_mutex_unlock(&g_contexts_lock);
#endif
    return ERR_NO_FREE_SLOT;
}

/*
 * antnet_run_iteration
 * Increments the iteration counter in a thread-safe manner.
 */
int antnet_run_iteration(int context_id)
{
    AntNetContext* ctx = get_context_by_id(context_id);
    if (!ctx) return ERR_INVALID_CONTEXT;
#ifndef _WIN32
    pthread_mutex_lock(&ctx->lock);
#endif
    ctx->iteration++;
#ifndef _WIN32
    pthread_mutex_unlock(&ctx->lock);
#endif
    return ERR_SUCCESS;
}

/*
 * antnet_get_best_path
 * Retrieves the current best path from the random solver, or returns a mock path
 * if none found. Thread-safe. (ACO & BF best path can be retrieved separately
 * if needed, or replaced with a param in future.)
 */
int antnet_get_best_path(
    int context_id,
    int* out_nodes,
    int max_size,
    int* out_path_len,
    int* out_total_latency
)
{
    if (!out_nodes || !out_path_len || !out_total_latency) {
        return ERR_INVALID_ARGS;
    }
    AntNetContext* ctx = get_context_by_id(context_id);
    if (!ctx) return ERR_INVALID_CONTEXT;

#ifndef _WIN32
    pthread_mutex_lock(&ctx->lock);
#endif
    if (ctx->random_best_length > 0) {
        if (ctx->random_best_length > max_size) {
#ifndef _WIN32
            pthread_mutex_unlock(&ctx->lock);
#endif
            return ERR_ARRAY_TOO_SMALL;
        }
        memcpy(out_nodes, ctx->random_best_nodes, ctx->random_best_length * sizeof(int));
        *out_path_len      = ctx->random_best_length;
        *out_total_latency = ctx->random_best_latency;
    } else {
        static int mock_nodes[] = {1, 2, 3, 5, 7, 9};
        int length = (int)(sizeof(mock_nodes) / sizeof(mock_nodes[0]));
        if (length > max_size) {
#ifndef _WIN32
            pthread_mutex_unlock(&ctx->lock);
#endif
            return ERR_ARRAY_TOO_SMALL;
        }
        memcpy(out_nodes, mock_nodes, length * sizeof(mock_nodes[0]));
        *out_path_len      = length;
        *out_total_latency = 42 + ctx->iteration; /* mock value */
    }
#ifndef _WIN32
    pthread_mutex_unlock(&ctx->lock);
#endif
    return ERR_SUCCESS;
}

/*
 * antnet_shutdown
 * Frees all allocated memory associated with the context, destroys locks,
 * and marks the slot as unused. Thread-safe with final destruction after unlock.
 */
int antnet_shutdown(int context_id)
{
    AntNetContext* ctx = get_context_by_id(context_id);
    if (!ctx) return ERR_INVALID_CONTEXT;

#ifndef _WIN32
    pthread_mutex_lock(&ctx->lock);
#endif
    if (ctx->nodes) {
        free(ctx->nodes);
        ctx->nodes = NULL;
    }
    if (ctx->edges) {
        free(ctx->edges);
        ctx->edges = NULL;
    }
    /* ACO arrays */
    if (ctx->aco_v1.adjacency) {
        free(ctx->aco_v1.adjacency);
        ctx->aco_v1.adjacency = NULL;
    }
    if (ctx->aco_v1.pheromones) {
        free(ctx->aco_v1.pheromones);
        ctx->aco_v1.pheromones = NULL;
    }

    printf("[antnet_shutdown] context %d final iteration: %d\n", context_id, ctx->iteration);

#ifndef _WIN32
    pthread_mutex_unlock(&ctx->lock);
    pthread_mutex_destroy(&ctx->lock);
#endif

#ifndef _WIN32
    pthread_mutex_lock(&g_contexts_lock);
#endif
    g_context_in_use[context_id] = 0;
#ifndef _WIN32
    pthread_mutex_unlock(&g_contexts_lock);
#endif
    return ERR_SUCCESS;
}

/*
 * NEW HELPER NOTE:
 * Each time a solver improves, the code calls update_on_improvement(...) for that solver.
 * Then it calls recalc_sasa_score(...) for the other solvers so that iteration-based factors
 * (like f = m / iteration) remain up to date, and the final ranking is updated accordingly.
 */

/*
 * antnet_run_all_solvers
 * Runs ACO, Random, and Brute-Force in sequence, each potentially improving its best path,
 * updates SASA states accordingly, and returns the best path found by each in out_* arrays.
 */
int antnet_run_all_solvers(
    int  context_id,
    int* out_nodes_aco,
    int  max_size_aco,
    int* out_len_aco,
    int* out_latency_aco,
    int* out_nodes_random,
    int  max_size_random,
    int* out_len_random,
    int* out_latency_random,
    int* out_nodes_brute,
    int  max_size_brute,
    int* out_len_brute,
    int* out_latency_brute
)
{
    if (!out_nodes_aco || !out_len_aco || !out_latency_aco ||
        !out_nodes_random || !out_len_random || !out_latency_random ||
        !out_nodes_brute || !out_len_brute || !out_latency_brute)
    {
        return ERR_INVALID_ARGS;
    }

    AntNetContext* ctx = get_context_by_id(context_id);
    if (!ctx) return ERR_INVALID_CONTEXT;

#ifndef _WIN32
    pthread_mutex_lock(&ctx->lock);
#endif
    /*
     * Increment iteration for each call to run_all_solvers.
     * This ensures SASA "iter_idx" moves forward properly.
     */
    ctx->iteration++;

    *out_len_aco     = 0;
    *out_latency_aco = 0;

    /* 1) ACO */
    {
        int old_aco_latency = (ctx->aco_best_length > 0) ? ctx->aco_best_latency : INT_MAX;
        int rc = aco_v1_run_iteration(ctx);
        if (rc != ERR_SUCCESS && rc != ERR_NO_TOPOLOGY) {
#ifndef _WIN32
            pthread_mutex_unlock(&ctx->lock);
#endif
            return rc;
        }
        rc = aco_v1_search_path(ctx, 0, 1,
                                out_nodes_aco, max_size_aco,
                                out_len_aco, out_latency_aco);
        if (rc != ERR_SUCCESS && rc != ERR_NO_PATH_FOUND) {
#ifndef _WIN32
            pthread_mutex_unlock(&ctx->lock);
#endif
            return rc;
        }

        /* If improved, update SASA for ACO, then recalc the other solvers' scores. */
        if (ctx->aco_best_length > 0 && ctx->aco_best_latency < old_aco_latency) {
            update_on_improvement(
                ctx->iteration,
                (double)ctx->aco_best_latency,
                &ctx->aco_sasa,
                ctx->sasa_coeffs.alpha,
                ctx->sasa_coeffs.beta,
                ctx->sasa_coeffs.gamma
            );

            recalc_sasa_score(&ctx->random_sasa, ctx->iteration,
                              ctx->sasa_coeffs.alpha,
                              ctx->sasa_coeffs.beta,
                              ctx->sasa_coeffs.gamma);
            recalc_sasa_score(&ctx->brute_sasa, ctx->iteration,
                              ctx->sasa_coeffs.alpha,
                              ctx->sasa_coeffs.beta,
                              ctx->sasa_coeffs.gamma);

            SasaState states[3] = {ctx->aco_sasa, ctx->random_sasa, ctx->brute_sasa};
            int rank_order[3];
            compute_ranking(states, 3, rank_order);
            printf("[RANK] ACO improved => order: #%d first, #%d second, #%d third\n",
                   rank_order[0], rank_order[1], rank_order[2]);
        }
    }

    /* 2) RANDOM */
    {
        int old_random_latency = (ctx->random_best_length > 0) ? ctx->random_best_latency : INT_MAX;
        int rc = random_search_path(
            ctx, 0, 1,
            out_nodes_random, max_size_random,
            out_len_random, out_latency_random
        );
        if (rc != ERR_SUCCESS && rc != ERR_NO_PATH_FOUND) {
#ifndef _WIN32
            pthread_mutex_unlock(&ctx->lock);
#endif
            return rc;
        }

        if (ctx->random_best_length > 0 && ctx->random_best_latency < old_random_latency) {
            update_on_improvement(
                ctx->iteration,
                (double)ctx->random_best_latency,
                &ctx->random_sasa,
                ctx->sasa_coeffs.alpha,
                ctx->sasa_coeffs.beta,
                ctx->sasa_coeffs.gamma
            );

            recalc_sasa_score(&ctx->aco_sasa, ctx->iteration,
                              ctx->sasa_coeffs.alpha,
                              ctx->sasa_coeffs.beta,
                              ctx->sasa_coeffs.gamma);
            recalc_sasa_score(&ctx->brute_sasa, ctx->iteration,
                              ctx->sasa_coeffs.alpha,
                              ctx->sasa_coeffs.beta,
                              ctx->sasa_coeffs.gamma);

            SasaState states[3] = {ctx->aco_sasa, ctx->random_sasa, ctx->brute_sasa};
            int rank_order[3];
            compute_ranking(states, 3, rank_order);
            printf("[RANK] Random improved => order: #%d first, #%d second, #%d third\n",
                   rank_order[0], rank_order[1], rank_order[2]);
        }
    }

    /* 3) BRUTE */
    {
        int old_brute_latency = (ctx->brute_best_length > 0) ? ctx->brute_best_latency : INT_MAX;
        int rc = brute_force_search_step(
            ctx, 0, 1,
            out_nodes_brute, max_size_brute,
            out_len_brute, out_latency_brute
        );
        if (rc != ERR_SUCCESS && rc != ERR_NO_PATH_FOUND) {
#ifndef _WIN32
            pthread_mutex_unlock(&ctx->lock);
#endif
            return rc;
        }

        if (ctx->brute_best_length > 0 && ctx->brute_best_latency < old_brute_latency) {
            update_on_improvement(
                ctx->iteration,
                (double)ctx->brute_best_latency,
                &ctx->brute_sasa,
                ctx->sasa_coeffs.alpha,
                ctx->sasa_coeffs.beta,
                ctx->sasa_coeffs.gamma
            );

            recalc_sasa_score(&ctx->aco_sasa, ctx->iteration,
                              ctx->sasa_coeffs.alpha,
                              ctx->sasa_coeffs.beta,
                              ctx->sasa_coeffs.gamma);
            recalc_sasa_score(&ctx->random_sasa, ctx->iteration,
                              ctx->sasa_coeffs.alpha,
                              ctx->sasa_coeffs.beta,
                              ctx->sasa_coeffs.gamma);

            SasaState states[3] = {ctx->aco_sasa, ctx->random_sasa, ctx->brute_sasa};
            int rank_order[3];
            compute_ranking(states, 3, rank_order);
            printf("[RANK] Brute improved => order: #%d first, #%d second, #%d third\n",
                   rank_order[0], rank_order[1], rank_order[2]);
        }
    }

#ifndef _WIN32
    pthread_mutex_unlock(&ctx->lock);
#endif
    return ERR_SUCCESS;
}

/*
 * antnet_init_from_config
 * Loads a config file, then calls antnet_initialize with those parameters.
 * On success, the context's config is updated. Thread-safe once context is created.
 */
int antnet_init_from_config(const char* config_path)
{
    if (!config_path) return ERR_INVALID_ARGS;
    AppConfig tmpcfg;
    config_set_defaults(&tmpcfg);

    if (!config_load(&tmpcfg, config_path)) {
        return -1;
    }
    int context_id = antnet_initialize(
        tmpcfg.set_nb_nodes,
        tmpcfg.min_hops,
        tmpcfg.max_hops
    );
    if (context_id < 0) return context_id;

    AntNetContext* ctx = get_context_by_id(context_id);
    if (!ctx) return ERR_INVALID_CONTEXT;

#ifndef _WIN32
    pthread_mutex_lock(&ctx->lock);
#endif
    ctx->config = tmpcfg;
#ifndef _WIN32
    pthread_mutex_unlock(&ctx->lock);
#endif
    return context_id;
}

/*
 * antnet_get_config
 * Thread-safe read of the current context config.
 */
int antnet_get_config(int context_id, AppConfig* out)
{
    if (!out) return ERR_INVALID_ARGS;

    AntNetContext* ctx = get_context_by_id(context_id);
    if (!ctx) return ERR_INVALID_CONTEXT;

#ifndef _WIN32
    pthread_mutex_lock(&ctx->lock);
#endif
    *out = ctx->config;
#ifndef _WIN32
    pthread_mutex_unlock(&ctx->lock);
#endif

    return ERR_SUCCESS;
}

/*
 * antnet_get_pheromone_matrix
 * Copies up to n*n floats into 'out', returns the total count on success,
 * negative on errors. Thread-safe read.
 */
int antnet_get_pheromone_matrix(int context_id, float* out, int max_count)
{
    AntNetContext* ctx = get_context_by_id(context_id);
    if (!ctx) return ERR_INVALID_CONTEXT;

#ifndef _WIN32
    pthread_mutex_lock(&ctx->lock);
#endif
    if (!ctx->aco_v1.pheromones || ctx->aco_v1.pheromone_size <= 0) {
#ifndef _WIN32
        pthread_mutex_unlock(&ctx->lock);
#endif
        return ERR_NO_TOPOLOGY;
    }
    int n = ctx->aco_v1.pheromone_size;
    int count = n * n;
    if (max_count < count) {
#ifndef _WIN32
        pthread_mutex_unlock(&ctx->lock);
#endif
        return ERR_ARRAY_TOO_SMALL;
    }
    memcpy(out, ctx->aco_v1.pheromones, sizeof(float) * count);
#ifndef _WIN32
    pthread_mutex_unlock(&ctx->lock);
#endif

    return count;
}

/*
 * antnet_render_heatmap_rgba
 *
 * GPU-accelerated offscreen heatmap rendering based on a cloud of 2D points
 * and their corresponding pheromone strength values. This function is fully
 * decoupled from AntNetContext and may be called from any thread.
 * It uses a persistent background renderer (hr_renderer_async).
 */
int antnet_render_heatmap_rgba(
    const float *pts_xy,
    const float *strength,
    int n,
    unsigned char *out_rgba,
    int width,
    int height
)
{
    if (!pts_xy || !strength || !out_rgba || n <= 0 || width <= 0 || height <= 0)
        return ERR_INVALID_ARGS;

    int rc = hr_enqueue_render(pts_xy, strength, n, out_rgba, width, height);
    if (rc != 0) {
        return ERR_INTERNAL_FAILURE;
    }
    return ERR_SUCCESS;
}

/*
 * antnet_renderer_async_init
 * Starts the persistent renderer thread if not already running. Returns 0 on success.
 */
int antnet_renderer_async_init(int initial_width, int initial_height)
{
    int ret = hr_renderer_start(initial_width, initial_height);
    if (ret != 0) {
        return ERR_INTERNAL_FAILURE;
    }
    return ERR_SUCCESS;
}

/*
 * antnet_renderer_async_shutdown
 * Stops the background renderer thread if running, cleans up. 
 * Safe to call multiple times.
 */
int antnet_renderer_async_shutdown(void)
{
    int ret = hr_renderer_stop();
    if (ret != 0) {
        return ERR_INTERNAL_FAILURE;
    }
    return ERR_SUCCESS;
}

/*
 * antnet_get_algo_ranking
 * Returns the list of algorithms sorted by SASA score in descending order.
 * The caller provides a RankingEntry array with size max_count.
 * The function writes up to 3 entries in 'out' (one per solver).
 * If max_count < 3, returns ERR_ARRAY_TOO_SMALL.
 * On success, returns the number of algorithms (3).
 */
int antnet_get_algo_ranking(int context_id, RankingEntry* out, int max_count)
{
    if (!out) {
        return ERR_INVALID_ARGS;
    }
    if (max_count < 3) {
        return ERR_ARRAY_TOO_SMALL;
    }

    AntNetContext* ctx = get_context_by_id(context_id);
    if (!ctx) return ERR_INVALID_CONTEXT;

#ifndef _WIN32
    pthread_mutex_lock(&ctx->lock);
#endif

    SasaState states[3];
    states[0] = ctx->aco_sasa;
    states[1] = ctx->random_sasa;
    states[2] = ctx->brute_sasa;

    int rank[3];
    compute_ranking(states, 3, rank);

    RankingEntry local[3];
    memset(local, 0, sizeof(local));

    /* Fill local[] in descending order of score based on rank[] */
    for (int i = 0; i < 3; i++) {
        int solver_idx = rank[i];
        if (solver_idx == 0) {
            strncpy(local[i].name, "ACO", sizeof(local[i].name) - 1);
            local[i].score = ctx->aco_sasa.score;
            local[i].latency_ms = ctx->aco_best_latency;
        } else if (solver_idx == 1) {
            strncpy(local[i].name, "RANDOM", sizeof(local[i].name) - 1);
            local[i].score = ctx->random_sasa.score;
            local[i].latency_ms = ctx->random_best_latency;
        } else {
            strncpy(local[i].name, "BRUTE", sizeof(local[i].name) - 1);
            local[i].score = ctx->brute_sasa.score;
            local[i].latency_ms = ctx->brute_best_latency;
        }
    }

    memcpy(out, local, sizeof(local));

#ifndef _WIN32
    pthread_mutex_unlock(&ctx->lock);
#endif

    return 3;
}

/*
 * antnet_set_sasa_params
 * Updates the SASA coefficients (alpha, beta, gamma) in a thread-safe manner.
 */
int antnet_set_sasa_params(int context_id, double alpha, double beta, double gamma)
{
    AntNetContext* ctx = get_context_by_id(context_id);
    if (!ctx) return ERR_INVALID_CONTEXT;

#ifndef _WIN32
    pthread_mutex_lock(&ctx->lock);
#endif
    ctx->sasa_coeffs.alpha = alpha;
    ctx->sasa_coeffs.beta  = beta;
    ctx->sasa_coeffs.gamma = gamma;
#ifndef _WIN32
    pthread_mutex_unlock(&ctx->lock);
#endif

    return ERR_SUCCESS;
}

/*
 * antnet_get_sasa_params
 * Reads the SASA coefficients (alpha, beta, gamma) in a thread-safe manner.
 */
int antnet_get_sasa_params(int context_id, double* out_alpha, double* out_beta, double* out_gamma)
{
    if (!out_alpha || !out_beta || !out_gamma) {
        return ERR_INVALID_ARGS;
    }

    AntNetContext* ctx = get_context_by_id(context_id);
    if (!ctx) return ERR_INVALID_CONTEXT;

#ifndef _WIN32
    pthread_mutex_lock(&ctx->lock);
#endif
    *out_alpha = ctx->sasa_coeffs.alpha;
    *out_beta  = ctx->sasa_coeffs.beta;
    *out_gamma = ctx->sasa_coeffs.gamma;
#ifndef _WIN32
    pthread_mutex_unlock(&ctx->lock);
#endif

    return ERR_SUCCESS;
}

/*
 * antnet_set_aco_params
 * Updates ACO parameters (alpha, beta, Q, evaporation, num_ants) in a thread-safe manner.
 * If num_ants <= 0, forces single-ant mode (num_ants=1).
 */
int antnet_set_aco_params(int context_id, float alpha, float beta, float Q, float evaporation, int num_ants)
{
    AntNetContext* ctx = get_context_by_id(context_id);
    if (!ctx) return ERR_INVALID_CONTEXT;

#ifndef _WIN32
    pthread_mutex_lock(&ctx->lock);
#endif
    ctx->aco_v1.alpha       = alpha;
    ctx->aco_v1.beta        = beta;
    ctx->aco_v1.Q           = Q;
    ctx->aco_v1.evaporation = evaporation;

    if (num_ants <= 0) {
        ctx->aco_v1.num_ants = 1;
    } else {
        ctx->aco_v1.num_ants = num_ants;
    }
#ifndef _WIN32
    pthread_mutex_unlock(&ctx->lock);
#endif

    return ERR_SUCCESS;
}

/*
 * antnet_get_aco_params
 * Reads alpha, beta, Q, evaporation, num_ants in a thread-safe manner.
 */
int antnet_get_aco_params(
    int context_id,
    float* out_alpha,
    float* out_beta,
    float* out_Q,
    float* out_evaporation,
    int*  out_num_ants
)
{
    if (!out_alpha || !out_beta || !out_Q || !out_evaporation || !out_num_ants) {
        return ERR_INVALID_ARGS;
    }

    AntNetContext* ctx = get_context_by_id(context_id);
    if (!ctx) return ERR_INVALID_CONTEXT;

#ifndef _WIN32
    pthread_mutex_lock(&ctx->lock);
#endif
    *out_alpha       = ctx->aco_v1.alpha;
    *out_beta        = ctx->aco_v1.beta;
    *out_Q           = ctx->aco_v1.Q;
    *out_evaporation = ctx->aco_v1.evaporation;
    *out_num_ants    = ctx->aco_v1.num_ants;
#ifndef _WIN32
    pthread_mutex_unlock(&ctx->lock);
#endif

    return ERR_SUCCESS;
}
