/* Relative Path: include/managers/config_manager.h */

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <stdbool.h>
#include "../types/antnet_config_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * pub_config_set_defaults
 * Initializes default values for all AppConfig fields.
 * This function does not perform any file I/O.
 */
void pub_config_set_defaults(AppConfig* cfg);

/*
 * pub_config_load
 * Loads the .ini file at the given path into *cfg.
 * Returns true on success, false on error (file open or parse failure).
 * Default values can be initialized beforehand with pub_config_set_defaults.
 */
bool pub_config_load(AppConfig* cfg, const char* filepath);

/*
 * pub_config_save
 * Writes fields from *cfg to the .ini file at filepath.
 * Returns true on success, false otherwise.
 * Overwrites any existing file content.
 */
bool pub_config_save(const AppConfig* cfg, const char* filepath);

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_MANAGER_H */
