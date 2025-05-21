/* Relative Path: src/c/core/score_evaluation.c */
/*
 * Implements the incremental SASA scoring logic and a helper function
 * to rank multiple algorithms by their SASA score.
 */

#include "../../../include/core/score_evaluation.h"
#include <float.h>  /* for DBL_MAX */
#include <math.h>

/*
 * priv_init_sasa_state
 * Initializes the SASA state fields to default values (best_L = +∞).
 */
void priv_init_sasa_state(SasaState *state)
{
    if (!state) return;
    state->best_L = DBL_MAX;
    state->last_improve_iter = 0;
    state->m = 0;
    state->sum_tau = 0.0;
    state->sum_r = 0.0;
    state->score = 0.0;
}

/*
 * priv_update_on_improvement
 * Called only if new_latency < state->best_L.
 * Applies the incremental SASA formula with the coefficients alpha, beta, gamma.
 */
void priv_update_on_improvement(
    int iter_idx,
    double new_latency,
    SasaState *state,
    double alpha,
    double beta,
    double gamma
)
{
    if (!state || iter_idx < 1) {
        /* No update if iteration index is invalid */
        return;
    }

    double prev_latency = state->best_L;
    if (prev_latency <= 0.0 || prev_latency == DBL_MAX) {
        /*
         * Fallback scenario if no valid best_L existed,
         * for the first improvement event.
         */
        prev_latency = new_latency * 2.0;
    }

    double tau = (double)(iter_idx - state->last_improve_iter);
    double r = (prev_latency - new_latency) / prev_latency;
    if (r < 0.0) {
        r = 0.0; /* safety clamp if floating calculation yields negative */
    }

    state->best_L = new_latency;
    state->last_improve_iter = iter_idx;
    state->m += 1;
    state->sum_tau += tau;
    state->sum_r   += r;

    /*
     * Recompute final SASA score after this improvement event.
     */
    {
        double m_d = (double)state->m;
        double tau_bar = (m_d > 0.0) ? (state->sum_tau / m_d) : 0.0;
        double r_bar   = (m_d > 0.0) ? (state->sum_r  / m_d) : 0.0;
        double f       = (iter_idx > 0) ? (m_d / (double)iter_idx) : 0.0;
        double v_tau   = (tau_bar <= 1e-9) ? 0.0 : (1.0 / tau_bar);

        state->score = alpha * v_tau + beta * r_bar + gamma * f;
    }
}

/*
 * NEW: priv_recalc_sasa_score
 * Recomputes the final SASA score in the same manner as priv_update_on_improvement,
 * but does NOT modify best_L, sum_r, sum_tau, m, or last_improve_iter.
 * This is used after *another* solver's improvement to refresh the iteration-based
 * weighting for solvers that have not improved in this iteration.
 */
void priv_recalc_sasa_score(
    SasaState *state,
    int iter_idx,
    double alpha,
    double beta,
    double gamma
)
{
    if (!state) return;

    double m_d = (double)state->m;
    if (m_d < 1e-9) {
        /* No improvements have ever occurred for this solver => score stays 0. */
        state->score = 0.0;
        return;
    }

    double tau_bar = state->sum_tau / m_d;
    double r_bar   = state->sum_r  / m_d;
    double f       = (iter_idx > 0) ? (m_d / (double)iter_idx) : 0.0;
    double v_tau   = (tau_bar <= 1e-9) ? 0.0 : (1.0 / tau_bar);

    state->score = alpha * v_tau + beta * r_bar + gamma * f;
}

/*
 * priv_compute_ranking
 * Ranks a set of algorithms by their SASA score in descending order.
 * A simple bubble sort is sufficient for a small number of algorithms.
 */
void priv_compute_ranking(const SasaState *states, int count, int *rank_out)
{
    if (!states || !rank_out || count <= 0) {
        return;
    }
    /* initialize rank array as [0,1,2,...] */
    for (int i = 0; i < count; i++) {
        rank_out[i] = i;
    }
    /* sort by states[].score descending */
    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            int ri = rank_out[i];
            int rj = rank_out[j];
            if (states[rj].score > states[ri].score) {
                int tmp = rank_out[i];
                rank_out[i] = rank_out[j];
                rank_out[j] = tmp;
            }
        }
    }
}
