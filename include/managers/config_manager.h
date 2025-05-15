// include/config_manager.h
#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <stdbool.h>
#include "./types/antnet_config_types.h" /* for AppConfig */

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_MANAGER_H */
