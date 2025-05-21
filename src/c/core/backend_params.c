/* Relative Path: src/c/core/backend_params.c */
/*
 * backend_params.c
 * Implements configuration and parameter-related API.
 */

#include "../../../include/core/backend_params.h"
#include "../../../include/managers/config_manager.h"
#include "../../../include/managers/ranking_manager.h"
#include "../../../include/rendering/heatmap_renderer_async.h"
#include "../../../include/consts/error_codes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * pub_init_from_config
 * Loads a config file, then calls pub_initialize with those parameters.
 * On success, the context's config is updated. Thread-safe once context is created.
 */
int pub_init_from_config(const char* config_path)
{
    if (!config_path)
    {
        return ERR_INVALID_ARGS;
    }
    AppConfig tmpcfg;
    pub_config_set_defaults(&tmpcfg);

    if (!pub_config_load(&tmpcfg, config_path))
    {
        return -1;
    }
    int context_id = pub_initialize(
        tmpcfg.set_nb_nodes,
        tmpcfg.min_hops,
        tmpcfg.max_hops
    );
    if (context_id < 0)
    {
        return context_id;
    }

    AntNetContext* ctx = priv_get_context_by_id(context_id);
    if (!ctx)
    {
        return ERR_INVALID_CONTEXT;
    }

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
 * pub_get_config
 * Thread-safe read of the current context config.
 */
int pub_get_config(int context_id, AppConfig* out)
{
    if (!out)
    {
        return ERR_INVALID_ARGS;
    }

    AntNetContext* ctx = priv_get_context_by_id(context_id);
    if (!ctx)
    {
        return ERR_INVALID_CONTEXT;
    }

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
 * pub_get_pheromone_matrix
 * Copies up to n*n floats into 'out', returns the total count on success,
 * negative on errors. Thread-safe read.
 */
int pub_get_pheromone_matrix(int context_id, float* out, int max_count)
{
    AntNetContext* ctx = priv_get_context_by_id(context_id);
    if (!ctx)
    {
        return ERR_INVALID_CONTEXT;
    }

#ifndef _WIN32
    pthread_mutex_lock(&ctx->lock);
#endif
    if (!ctx->aco_v1.pheromones || ctx->aco_v1.pheromone_size <= 0)
    {
#ifndef _WIN32
        pthread_mutex_unlock(&ctx->lock);
#endif
        return ERR_NO_TOPOLOGY;
    }
    int n = ctx->aco_v1.pheromone_size;
    int count = n * n;
    if (max_count < count)
    {
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
 * pub_get_algo_ranking
 * Returns the list of algorithms sorted by SASA score in descending order.
 * The caller provides a RankingEntry array with size max_count.
 * The function writes up to 3 entries in 'out' (one per solver).
 * If max_count < 3, returns ERR_ARRAY_TOO_SMALL.
 * On success, returns the number of algorithms (3).
 */
int pub_get_algo_ranking(int context_id, RankingEntry* out, int max_count)
{
    if (!out)
    {
        return ERR_INVALID_ARGS;
    }
    if (max_count < 3)
    {
        return ERR_ARRAY_TOO_SMALL;
    }

    AntNetContext* ctx = priv_get_context_by_id(context_id);
    if (!ctx)
    {
        return ERR_INVALID_CONTEXT;
    }

#ifndef _WIN32
    pthread_mutex_lock(&ctx->lock);
#endif

    SasaState states[3];
    states[0] = ctx->aco_sasa;
    states[1] = ctx->random_sasa;
    states[2] = ctx->brute_sasa;

    int rank[3];
    priv_compute_ranking(states, 3, rank);

    RankingEntry local[3];
    memset(local, 0, sizeof(local));

    /* Fill local[] in descending order of score based on rank[] */
    for (int i = 0; i < 3; i++)
    {
        int solver_idx = rank[i];
        if (solver_idx == 0)
        {
            strncpy(local[i].name, "ACO", sizeof(local[i].name) - 1);
            local[i].score = ctx->aco_sasa.score;
            local[i].latency_ms = ctx->aco_best_latency;
        }
        else if (solver_idx == 1)
        {
            strncpy(local[i].name, "RANDOM", sizeof(local[i].name) - 1);
            local[i].score = ctx->random_sasa.score;
            local[i].latency_ms = ctx->random_best_latency;
        }
        else
        {
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
 * pub_set_sasa_params
 * Updates the SASA coefficients (alpha, beta, gamma) in a thread-safe manner.
 */
int pub_set_sasa_params(int context_id, double alpha, double beta, double gamma)
{
    AntNetContext* ctx = priv_get_context_by_id(context_id);
    if (!ctx)
    {
        return ERR_INVALID_CONTEXT;
    }

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
 * pub_get_sasa_params
 * Reads the SASA coefficients (alpha, beta, gamma) in a thread-safe manner.
 */
int pub_get_sasa_params(int context_id, double* out_alpha, double* out_beta, double* out_gamma)
{
    if (!out_alpha || !out_beta || !out_gamma)
    {
        return ERR_INVALID_ARGS;
    }

    AntNetContext* ctx = priv_get_context_by_id(context_id);
    if (!ctx)
    {
        return ERR_INVALID_CONTEXT;
    }

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
 * pub_set_aco_params
 * Updates ACO parameters (alpha, beta, Q, evaporation, num_ants) in a thread-safe manner.
 * If num_ants <= 0, forces single-ant mode (num_ants=1).
 */
int pub_set_aco_params(int context_id, float alpha, float beta, float Q,
                          float evaporation, int num_ants)
{
    AntNetContext* ctx = priv_get_context_by_id(context_id);
    if (!ctx)
    {
        return ERR_INVALID_CONTEXT;
    }

#ifndef _WIN32
    pthread_mutex_lock(&ctx->lock);
#endif
    ctx->aco_v1.alpha       = alpha;
    ctx->aco_v1.beta        = beta;
    ctx->aco_v1.Q           = Q;
    ctx->aco_v1.evaporation = evaporation;

    if (num_ants <= 0)
    {
        ctx->aco_v1.num_ants = 1;
    }
    else
    {
        ctx->aco_v1.num_ants = num_ants;
    }
#ifndef _WIN32
    pthread_mutex_unlock(&ctx->lock);
#endif

    return ERR_SUCCESS;
}

/*
 * pub_get_aco_params
 * Reads alpha, beta, Q, evaporation, num_ants in a thread-safe manner.
 */
int pub_get_aco_params(
    int context_id,
    float* out_alpha,
    float* out_beta,
    float* out_Q,
    float* out_evaporation,
    int*  out_num_ants
)
{
    if (!out_alpha || !out_beta || !out_Q || !out_evaporation || !out_num_ants)
    {
        return ERR_INVALID_ARGS;
    }

    AntNetContext* ctx = priv_get_context_by_id(context_id);
    if (!ctx)
    {
        return ERR_INVALID_CONTEXT;
    }

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
