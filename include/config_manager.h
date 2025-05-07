// include/config_manager.h
#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <stdbool.h>

/*
 * AppConfig: holds all configuration parameters used throughout the project.
 * The config_load and config_save functions manipulate these fields.
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

/*
 * config_load: reads configuration from the .ini file at filepath and updates *cfg.
 * Returns true on success, false on error (e.g., file not found, parse error).
 * Thread-safe: uses an internal mutex to prevent race conditions during file I/O.
 */
bool config_load(AppConfig* cfg, const char* filepath);

/*
 * config_save: writes the fields in *cfg to the .ini file at filepath.
 * Returns true on success, false on error (e.g., write failure).
 * Thread-safe: uses an internal mutex to prevent race conditions during file I/O.
 */
bool config_save(const AppConfig* cfg, const char* filepath);

/*
 * config_set_defaults: sets default values in *cfg for all fields.
 * Callers can override some/all of these values manually or via config_load.
 */
void config_set_defaults(AppConfig* cfg);

#endif // CONFIG_MANAGER_H
