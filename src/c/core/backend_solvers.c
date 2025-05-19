/* Relative Path: src/c/core/backend_solvers.c */
/*
 * backend_solvers.c
 * Implements solver orchestration functions such as antnet_run_iteration,
 * antnet_run_all_solvers, and retrieval of best path from the random solver.
 */

#include "../../../include/core/backend_solvers.h"
#include "../../../include/managers/cpu_acoV1_algo_manager.h"
#include "../../../include/managers/cpu_random_algo_manager.h"
#include "../../../include/managers/cpu_brute_force_algo_manager.h"
#include "../../../include/core/score_evaluation.h"
#include "../../../include/consts/error_codes.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>

/*
 * antnet_run_iteration
 * Increments the iteration counter in a thread-safe manner.
 */
int antnet_run_iteration(int context_id)
{
    AntNetContext* ctx = get_context_by_id(context_id);
    if (!ctx)
    {
        return ERR_INVALID_CONTEXT;
    }
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
    if (!out_nodes || !out_path_len || !out_total_latency)
    {
        return ERR_INVALID_ARGS;
    }
    AntNetContext* ctx = get_context_by_id(context_id);
    if (!ctx)
    {
        return ERR_INVALID_CONTEXT;
    }

#ifndef _WIN32
    pthread_mutex_lock(&ctx->lock);
#endif
    if (ctx->random_best_length > 0)
    {
        if (ctx->random_best_length > max_size)
        {
#ifndef _WIN32
            pthread_mutex_unlock(&ctx->lock);
#endif
            return ERR_ARRAY_TOO_SMALL;
        }
        memcpy(out_nodes,
               ctx->random_best_nodes,
               ctx->random_best_length * sizeof(int));
        *out_path_len      = ctx->random_best_length;
        *out_total_latency = ctx->random_best_latency;
    }
    else
    {
        static int mock_nodes[] = {1, 2, 3, 5, 7, 9};
        int length = (int)(sizeof(mock_nodes) / sizeof(mock_nodes[0]));
        if (length > max_size)
        {
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
    if (!ctx)
    {
        return ERR_INVALID_CONTEXT;
    }

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
        int rc = aco_algo_manager_run_iteration(ctx);
        if (rc != ERR_SUCCESS && rc != ERR_NO_TOPOLOGY)
        {
#ifndef _WIN32
            pthread_mutex_unlock(&ctx->lock);
#endif
            return rc;
        }
        rc = aco_algo_manager_search_path(ctx, 0, 1,
                                          out_nodes_aco, max_size_aco,
                                          out_len_aco, out_latency_aco);
        if (rc != ERR_SUCCESS && rc != ERR_NO_PATH_FOUND)
        {
#ifndef _WIN32
            pthread_mutex_unlock(&ctx->lock);
#endif
            return rc;
        }

        /* If improved, update SASA for ACO, then recalc the other solvers' scores. */
        if (ctx->aco_best_length > 0 && ctx->aco_best_latency < old_aco_latency)
        {
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
        int rc = random_algo_manager_run(
            ctx, 0, 1,
            out_nodes_random, max_size_random,
            out_len_random, out_latency_random
        );
        if (rc != ERR_SUCCESS && rc != ERR_NO_PATH_FOUND)
        {
#ifndef _WIN32
            pthread_mutex_unlock(&ctx->lock);
#endif
            return rc;
        }

        if (ctx->random_best_length > 0 && ctx->random_best_latency < old_random_latency)
        {
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
        int rc = brute_force_algo_manager_run(
            ctx, 0, 1,
            out_nodes_brute, max_size_brute,
            out_len_brute, out_latency_brute
        );
        if (rc != ERR_SUCCESS && rc != ERR_NO_PATH_FOUND)
        {
#ifndef _WIN32
            pthread_mutex_unlock(&ctx->lock);
#endif
            return rc;
        }

        if (ctx->brute_best_length > 0 && ctx->brute_best_latency < old_brute_latency)
        {
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
