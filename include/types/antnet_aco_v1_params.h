/* Relative Path: include/types/antnet_aco_v1_params.h */
/*
 * Defines the aco_v1_params_t struct for user-configurable ACO parameters.
 * Allows external control over alpha, beta, evaporation, Q, and the number of ants.
 * Includes a function to set these parameters in the corresponding context.
*/

#ifndef ANTNET_ACO_V1_PARAMS_H
#define ANTNET_ACO_V1_PARAMS_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * aco_v1_params_s: C struct matching the user-defined ACO parameters
 * that may be set from Python or a config file. This allows external
 * configuration of alpha, beta, evaporation, Q, and number of ants.
 */
typedef struct aco_v1_params_s {
    float alpha;
    float beta;
    float evaporation;
    float Q;
    int   num_ants;
} aco_v1_params_t;

/*
 * antnet_set_aco_v1_params: sets the ACO parameters for the given context,
 * copying them into the internal AcoV1State.
 * Returns 0 on success, negative on error.
 */
int antnet_set_aco_v1_params(int context_id, const aco_v1_params_t* params);

#ifdef __cplusplus
}
#endif

#endif /* ANTNET_ACO_V1_PARAMS_H */
