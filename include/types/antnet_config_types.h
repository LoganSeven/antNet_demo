/* Relative Path: include/types/antnet_config_types.h */
/*
 * Declares AppConfig struct, representing simulation and feature settings loaded from .ini files.
 * Holds fields for general simulation parameters, node configuration, DDoS scenarios, and more.
 * Acts as the central container for user-defined or default config values in AntNet.
*/

#ifndef ANTNET_CONFIG_TYPES_H
#define ANTNET_CONFIG_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/*
 * AppConfig holds configuration fields from the .ini file.
 * This struct is updated to reflect new names and additional fields.
 */
typedef struct AppConfig
{
    /* [simulation] */
    int nb_ants;         
    int set_nb_nodes;
    int min_hops;
    int max_hops;

    /* [node] */
    int default_min_delay;
    int default_max_delay;
    int death_delay;
    int under_attack_id;
    bool attack_started;

    /* [features] */
    bool simulate_ddos;
    bool show_random_performance;
    bool show_brute_performance;

    /* [ranking] */
    double ranking_alpha;
    double ranking_beta;
    double ranking_gamma;

    /* [ants] */
    float ant_alpha;
    float ant_beta;
    float ant_Q;
    float ant_evaporation;

} AppConfig;

#ifdef __cplusplus
}
#endif

#endif /* ANTNET_CONFIG_TYPES_H */
