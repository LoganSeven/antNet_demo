// src/c/config_manager.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>

#include "../../third_party/ini.h"      // Unmodified inih header
#include "../../include/config_manager.h"

/*
 * Internal mutex to ensure thread safety when loading or saving config files.
 */
static pthread_mutex_t g_config_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * config_set_defaults: sets fixed default values in the cfg structure.
 * This function does not perform any file I/O.
 */
void config_set_defaults(AppConfig* cfg)
{
    cfg->nb_swarms              = 3;
    cfg->set_nb_nodes           = 16;
    cfg->min_hops               = 3;
    cfg->max_hops               = 6;
    cfg->default_delay          = 30;
    cfg->death_delay            = 200;
    cfg->under_attack_id        = 5;
    cfg->attack_started         = true;
    cfg->simulate_ddos          = true;
    cfg->show_random_performance= true;
    cfg->show_brute_performance = false;
}

/*
 * parse_bool_value: converts a string to a bool.
 * Accepts "true"/"1" (case-insensitive) as true; "false"/"0" as false.
 * Returns the parsed bool or false if unrecognized.
 */
static bool parse_bool_value(const char* str)
{
    if (!str) return false;

    // Lowercase copy for easy comparison
    char buf[16];
    memset(buf, 0, sizeof(buf));
    int i = 0;
    while (str[i] && i < (int)(sizeof(buf) - 1)) {
        buf[i] = (char)tolower((unsigned char)str[i]);
        i++;
    }
    buf[i] = '\0';

    if (strcmp(buf, "true") == 0 || strcmp(buf, "1") == 0) {
        return true;
    }
    if (strcmp(buf, "false") == 0 || strcmp(buf, "0") == 0) {
        return false;
    }
    return false; /* fallback */
}

/*
 * config_ini_handler: callback used by ini_parse to fill the AppConfig struct.
 * user  = pointer to AppConfig
 * name  = name within a section (e.g. "nb_swarms", "simulate_ddos", etc.)
 * value = string found in .ini
 * Returns 1 on success, 0 if a parse error requires stopping.
 */
static int config_ini_handler(void* user, const char* section,
                              const char* name, const char* value)
{
    if (!user || !section || !name || !value) {
        return 0; // invalid pointer => parse error
    }

    AppConfig* cfg = (AppConfig*)user;

    /* section: "simulation" */
    if (strcmp(section, "simulation") == 0) {
        if      (strcmp(name, "nb_swarms")   == 0) { cfg->nb_swarms    = atoi(value); }
        else if (strcmp(name, "set_nb_nodes")== 0) { cfg->set_nb_nodes = atoi(value); }
        else if (strcmp(name, "min_hops")    == 0) { cfg->min_hops     = atoi(value); }
        else if (strcmp(name, "max_hops")    == 0) { cfg->max_hops     = atoi(value); }
    }
    /* section: "node" */
    else if (strcmp(section, "node") == 0) {
        if      (strcmp(name, "default_delay")   == 0) { cfg->default_delay   = atoi(value); }
        else if (strcmp(name, "death_delay")     == 0) { cfg->death_delay     = atoi(value); }
        else if (strcmp(name, "under_attack_id") == 0) { cfg->under_attack_id = atoi(value); }
        else if (strcmp(name, "attack_started")  == 0) { cfg->attack_started  = parse_bool_value(value); }
    }
    /* section: "features" */
    else if (strcmp(section, "features") == 0) {
        if      (strcmp(name, "simulate_ddos")          == 0) { cfg->simulate_ddos          = parse_bool_value(value); }
        else if (strcmp(name, "show_random_performance")== 0) { cfg->show_random_performance= parse_bool_value(value); }
        else if (strcmp(name, "show_brute_performance") == 0) { cfg->show_brute_performance = parse_bool_value(value); }
    }

    return 1; // continue parsing
}

/*
 * config_load: loads the .ini file at filepath into *cfg.
 * Returns true on success, false on error (e.g. file open or parse failure).
 * This function does not discard existing cfg fields if missing from the file.
 * Default values can be set beforehand with config_set_defaults if needed.
 */
bool config_load(AppConfig* cfg, const char* filepath)
{
    if (!cfg || !filepath) {
        return false;
    }

    pthread_mutex_lock(&g_config_mutex);

    /* parse ini */
    int parse_result = ini_parse(filepath, config_ini_handler, cfg);
    if (parse_result != 0) {
        /*
         * parse_result > 0 => line of first error
         * parse_result = -1 => file open error
         * parse_result = -2 => memory error
         */
        pthread_mutex_unlock(&g_config_mutex);
        return false;
    }

    pthread_mutex_unlock(&g_config_mutex);
    return true;
}

/*
 * config_save: writes fields from *cfg to the .ini file at filepath.
 * Returns true on success, false otherwise.
 * Overwrites existing file contents.
 */
bool config_save(const AppConfig* cfg, const char* filepath)
{
    if (!cfg || !filepath) {
        return false;
    }

    pthread_mutex_lock(&g_config_mutex);

    FILE* fp = fopen(filepath, "w");
    if (!fp) {
        pthread_mutex_unlock(&g_config_mutex);
        return false;
    }

    /*
     * Write out the config in standard .ini format:
     *   [simulation]
     *   ...
     *   [node]
     *   ...
     *   [features]
     *   ...
     */
    fprintf(fp, "[simulation]\n");
    fprintf(fp, "nb_swarms = %d\n",          cfg->nb_swarms);
    fprintf(fp, "set_nb_nodes = %d\n",       cfg->set_nb_nodes);
    fprintf(fp, "min_hops = %d\n",           cfg->min_hops);
    fprintf(fp, "max_hops = %d\n",           cfg->max_hops);
    fprintf(fp, "\n[node]\n");
    fprintf(fp, "default_delay = %d\n",      cfg->default_delay);
    fprintf(fp, "death_delay = %d\n",        cfg->death_delay);
    fprintf(fp, "under_attack_id = %d\n",    cfg->under_attack_id);
    fprintf(fp, "attack_started = %s\n",     cfg->attack_started ? "true" : "false");
    fprintf(fp, "\n[features]\n");
    fprintf(fp, "simulate_ddos = %s\n",          cfg->simulate_ddos ? "true" : "false");
    fprintf(fp, "show_random_performance = %s\n",cfg->show_random_performance ? "true" : "false");
    fprintf(fp, "show_brute_performance = %s\n", cfg->show_brute_performance ? "true" : "false");

    fclose(fp);

    pthread_mutex_unlock(&g_config_mutex);
    return true;
}
