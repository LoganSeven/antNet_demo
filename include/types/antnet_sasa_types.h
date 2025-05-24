/* Relative Path: include/types/antnet_sasa_types.h */
#ifndef ANTNET_SASA_TYPES_H
#define ANTNET_SASA_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * SasaCoeffs
 * Holds alpha, beta, gamma coefficients used by the SASA scoring logic.
 */
typedef struct SasaCoeffs {
    double alpha;
    double beta;
    double gamma;
} SasaCoeffs;

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

#ifdef __cplusplus
}
#endif

#endif /* ANTNET_SASA_TYPES_H */
