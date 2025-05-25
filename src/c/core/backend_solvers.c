/* Relative Path: src/c/core/backend_solvers.c */
/*
 * Orchestrates solver operations (ACO, Random, Brute-Force) and retrieves best paths.
 * Manages thread-safe iteration counters and aggregator logic for solver outputs.
 * Key file for coordinating multi-solver pathfinding in AntNet.
*/

#include "../../../include/core/backend_solvers.h"
#include "../../../include/managers/cpu_acoV1_algo_manager.h"
#include "../../../include/managers/cpu_random_algo_manager.h"
#include "../../../include/managers/cpu_brute_force_algo_manager.h"
#include "../../../include/managers/ranking_manager.h"
#include "../../../include/consts/error_codes.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>

/*
 * pub_run_iteration
 * Increments the iteration counter in a thread-safe manner.
 */
int pub_run_iteration(int context_id)
{
    AntNetContext* ctx = priv_get_context_by_id(context_id);
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
 * pub_get_best_path
 * Retrieves the current best path from the random solver, or returns a mock path
 * if none is available. Thread-safe. Retrieval from ACO and Brute-Force solvers
 * can be implemented separately or parameterized in the future.
 */
int pub_get_best_path(
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
    AntNetContext* ctx = priv_get_context_by_id(context_id);
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
 * pub_run_all_solvers
 * Executes the ACO, Random, and Brute-Force solvers in sequence.
 * Each solver may improve its internal best path. SASA states are updated accordingly,
 * and best paths from each algorithm are returned via the output arrays.
 *
 * OLD APPROACH:
 *     1) Held ctx->lock from start to end,
 *     2) Then called aco_v1_run_iteration_threaded, which tries locking again => deadlock.
 *
 * NEW APPROACH:
 *     1) Lock *briefly* to increment ctx->iteration,
 *     2) Unlock,
 *     3) Call each solver (they do internal locking),
 *     4) Re-lock only if needed for local tasks.
 */
int pub_run_all_solvers(
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

    AntNetContext* ctx = priv_get_context_by_id(context_id);
    if (!ctx)
    {
        return ERR_INVALID_CONTEXT;
    }

    /* Step 1: briefly lock just to increment iteration safely. */
#ifndef _WIN32
    pthread_mutex_lock(&ctx->lock);
#endif
    ctx->iteration++;
#ifndef _WIN32
    pthread_mutex_unlock(&ctx->lock);
#endif

    /* Initialize outputs for ACO. */
    *out_len_aco     = 0;
    *out_latency_aco = 0;

    /*
     * 2) ACO solver
     *    We do *not* hold ctx->lock here, to avoid nested lock in aco_shared_merge_deltas.
     */
    {
        int old_aco_latency = (ctx->aco_best_length > 0) ? ctx->aco_best_latency : INT_MAX;
        int rc = aco_algo_manager_run_iteration(ctx);
        if (rc != ERR_SUCCESS && rc != ERR_NO_TOPOLOGY)
        {
            return rc;
        }
        rc = aco_algo_manager_search_path(ctx, 0, 1,
                                          out_nodes_aco, max_size_aco,
                                          out_len_aco, out_latency_aco);
        if (rc != ERR_SUCCESS && rc != ERR_NO_PATH_FOUND)
        {
            return rc;
        }

        /*
         * For SASA updating, we do need ctx->lock, but let's lock *only* here
         * for the check & update. 
         */
#ifndef _WIN32
        pthread_mutex_lock(&ctx->lock);
#endif
        if (ctx->aco_best_length > 0 && ctx->aco_best_latency < old_aco_latency)
        {
            priv_update_on_improvement(
                ctx->iteration,
                (double)ctx->aco_best_latency,
                &ctx->aco_sasa,
                ctx->sasa_coeffs.alpha,
                ctx->sasa_coeffs.beta,
                ctx->sasa_coeffs.gamma
            );

            priv_recalc_sasa_score(&ctx->random_sasa, ctx->iteration,
                                   ctx->sasa_coeffs.alpha,
                                   ctx->sasa_coeffs.beta,
                                   ctx->sasa_coeffs.gamma);
            priv_recalc_sasa_score(&ctx->brute_sasa, ctx->iteration,
                                   ctx->sasa_coeffs.alpha,
                                   ctx->sasa_coeffs.beta,
                                   ctx->sasa_coeffs.gamma);

            SasaState states[3] = {ctx->aco_sasa, ctx->random_sasa, ctx->brute_sasa};
            int rank_order[3];
            priv_compute_ranking(states, 3, rank_order);
            printf("[RANK] ACO improved => order: #%d first, #%d second, #%d third\n",
                   rank_order[0], rank_order[1], rank_order[2]);
        }
#ifndef _WIN32
        pthread_mutex_unlock(&ctx->lock);
#endif
    }

    /*
     * 3) RANDOM solver (similar approach, do not hold ctx->lock across entire run)
     */
    {
        int old_random_latency;
#ifndef _WIN32
        pthread_mutex_lock(&ctx->lock);
#endif
        old_random_latency = (ctx->random_best_length > 0) ? ctx->random_best_latency : INT_MAX;
#ifndef _WIN32
        pthread_mutex_unlock(&ctx->lock);
#endif

        int rc = random_algo_manager_run(
            ctx, 0, 1,
            out_nodes_random, max_size_random,
            out_len_random, out_latency_random
        );
        if (rc != ERR_SUCCESS && rc != ERR_NO_PATH_FOUND)
        {
            return rc;
        }

        /*
         * If random improved, lock only for the SASA update portion.
         */
#ifndef _WIN32
        pthread_mutex_lock(&ctx->lock);
#endif
        if (ctx->random_best_length > 0 && ctx->random_best_latency < old_random_latency)
        {
            priv_update_on_improvement(
                ctx->iteration,
                (double)ctx->random_best_latency,
                &ctx->random_sasa,
                ctx->sasa_coeffs.alpha,
                ctx->sasa_coeffs.beta,
                ctx->sasa_coeffs.gamma
            );

            priv_recalc_sasa_score(&ctx->aco_sasa, ctx->iteration,
                                   ctx->sasa_coeffs.alpha,
                                   ctx->sasa_coeffs.beta,
                                   ctx->sasa_coeffs.gamma);
            priv_recalc_sasa_score(&ctx->brute_sasa, ctx->iteration,
                                   ctx->sasa_coeffs.alpha,
                                   ctx->sasa_coeffs.beta,
                                   ctx->sasa_coeffs.gamma);

            SasaState states[3] = {ctx->aco_sasa, ctx->random_sasa, ctx->brute_sasa};
            int rank_order[3];
            priv_compute_ranking(states, 3, rank_order);
            printf("[RANK] Random improved => order: #%d first, #%d second, #%d third\n",
                   rank_order[0], rank_order[1], rank_order[2]);
        }
#ifndef _WIN32
        pthread_mutex_unlock(&ctx->lock);
#endif
    }

    /*
     * 4) BRUTE solver (same pattern)
     */
    {
        int old_brute_latency;
#ifndef _WIN32
        pthread_mutex_lock(&ctx->lock);
#endif
        old_brute_latency = (ctx->brute_best_length > 0) ? ctx->brute_best_latency : INT_MAX;
#ifndef _WIN32
        pthread_mutex_unlock(&ctx->lock);
#endif

        int rc = brute_force_algo_manager_run(
            ctx, 0, 1,
            out_nodes_brute, max_size_brute,
            out_len_brute, out_latency_brute
        );
        if (rc != ERR_SUCCESS && rc != ERR_NO_PATH_FOUND)
        {
            return rc;
        }

#ifndef _WIN32
        pthread_mutex_lock(&ctx->lock);
#endif
        if (ctx->brute_best_length > 0 && ctx->brute_best_latency < old_brute_latency)
        {
            priv_update_on_improvement(
                ctx->iteration,
                (double)ctx->brute_best_latency,
                &ctx->brute_sasa,
                ctx->sasa_coeffs.alpha,
                ctx->sasa_coeffs.beta,
                ctx->sasa_coeffs.gamma
            );

            priv_recalc_sasa_score(&ctx->aco_sasa, ctx->iteration,
                                   ctx->sasa_coeffs.alpha,
                                   ctx->sasa_coeffs.beta,
                                   ctx->sasa_coeffs.gamma);
            priv_recalc_sasa_score(&ctx->random_sasa, ctx->iteration,
                                   ctx->sasa_coeffs.alpha,
                                   ctx->sasa_coeffs.beta,
                                   ctx->sasa_coeffs.gamma);

            SasaState states[3] = {ctx->aco_sasa, ctx->random_sasa, ctx->brute_sasa};
            int rank_order[3];
            priv_compute_ranking(states, 3, rank_order);
            printf("[RANK] Brute improved => order: #%d first, #%d second, #%d third\n",
                   rank_order[0], rank_order[1], rank_order[2]);
        }
#ifndef _WIN32
        pthread_mutex_unlock(&ctx->lock);
#endif
    }

    /* Done. No final context lock needed here. */
    return ERR_SUCCESS;
}
