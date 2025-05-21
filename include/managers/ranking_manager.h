/* Relative Path: include/managers/ranking_manager.h */
/*
 * Provides the incremental SASA scoring approach for ranking path-finder algorithms.
 * The struct SasaState holds the incremental metrics for one algorithm.
 */

#ifndef RANKING_MANAGER_H
#define RANKING_MANAGER_H

/*
 * SasaState
 * Holds the state for the incremental SASA scoring approach for one algorithm.
 */
typedef struct SasaState {
    double best_L;             /* best latency encountered so far */
    int    last_improve_iter;  /* iteration index when best_L was last improved */
    int    m;                  /* improvement counter */
    double sum_tau;            /* sum of intervals between improvements */
    double sum_r;              /* sum of relative gains */
    double score;              /* final composite score for ranking */
} SasaState;

/*
 * priv_init_sasa_state
 * Initializes the SASA state fields to default values (best_L = +∞, etc.)
 */
void priv_init_sasa_state(SasaState *state);

/*
 * priv_update_on_improvement
 * Called only if new_latency < state->best_L.
 * Applies the incremental SASA formula with the coefficients alpha, beta, gamma.
 * iter_idx: iteration number >= 1
 * new_latency: the newly found best latency
 */
void priv_update_on_improvement(
    int iter_idx,
    double new_latency,
    SasaState *state,
    double alpha,
    double beta,
    double gamma
);

/*
 * priv_recalc_sasa_score
 * Recomputes the final SASA score for a solver that did NOT improve on this iteration,
 * so that iteration-based factors (like improvement frequency f=m/iter) remain accurate.
 * Does not alter best_L, sum_r, sum_tau, m, or last_improve_iter.
 */
void priv_recalc_sasa_score(
    SasaState *state,
    int iter_idx,
    double alpha,
    double beta,
    double gamma
);

/*
 * priv_compute_ranking
 * Ranks a set of algorithms by their SASA score in descending order.
 * rank_out[i] will be the index of the i-th best scorer.
 * Example usage for 3 algorithms:
 *   SasaState array[3] = {aco_sasa, random_sasa, brute_sasa};
 *   int rank[3];
 *   priv_compute_ranking(array, 3, rank);
 *   => rank[0], rank[1], rank[2] in descending order of scores
 */
void priv_compute_ranking(const SasaState *states, int count, int *rank_out);

#endif /* RANKING_MANAGER_H */
