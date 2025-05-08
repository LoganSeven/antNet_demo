//include/antnet_config_types.h
#ifndef ANTNET_CONFIG_TYPES_H
#define ANTNET_CONFIG_TYPES_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * AppConfig: holds all configuration parameters used throughout the project.
 * The config_load and config_save functions manipulate these fields.
 * Moved here from config_manager.h to separate the data struct from the I/O routines.
 */
typedef struct {
    int  nb_swarms;
    int  set_nb_nodes;
    int  min_hops;
    int  max_hops;
    int  default_delay;
    int  death_delay;
    int  under_attack_id;
    bool attack_started;
    bool simulate_ddos;
    bool show_random_performance;
    bool show_brute_performance;
} AppConfig;

#ifdef __cplusplus
}
#endif

#endif /* ANTNET_CONFIG_TYPES_H */
