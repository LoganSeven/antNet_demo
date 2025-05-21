/* Relative Path: src/c/managers/config_manager.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifndef _WIN32
#include <pthread.h>
#endif

#include "../../../third_party/ini.h"      /* Unmodified inih header */
#include "../../../include/managers/config_manager.h"

/*
 * Internal mutex to ensure thread safety when loading or saving config files.
 */
#ifndef _WIN32
static pthread_mutex_t g_config_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

/*
 * pub_config_set_defaults: sets fixed default values in the cfg structure.
 * This function does not perform any file I/O.
 */
void pub_config_set_defaults(AppConfig* cfg)
{
    /* [simulation] defaults */
    cfg->nb_ants       = 3;
    cfg->set_nb_nodes  = 16;
    cfg->min_hops      = 3;
    cfg->max_hops      = 6;

    /* [node] defaults */
    cfg->default_min_delay = 3;
    cfg->default_max_delay = 250;
    cfg->death_delay       = 200;
    cfg->under_attack_id   = 5;
    cfg->attack_started    = true;

    /* [features] defaults */
    cfg->simulate_ddos           = true;
    cfg->show_random_performance = true;
    cfg->show_brute_performance  = false;

    /* [ranking] defaults */
    cfg->ranking_alpha = 0.4;
    cfg->ranking_beta  = 0.4;
    cfg->ranking_gamma = 0.2;

    /* [ants] defaults */
    cfg->ant_alpha       = 1.0f;
    cfg->ant_beta        = 2.0f;
    cfg->ant_Q           = 500.0f;
    cfg->ant_evaporation = 0.1f;
}

/*
 * parse_bool_value: converts a string to a bool.
 * Accepts "true"/"1" (case-insensitive) as true; "false"/"0" as false.
 * Returns the parsed bool or false if unrecognized.
 */
static bool parse_bool_value(const char* str)
{
    if (!str) return false;

    /* Lowercase copy for easy comparison */
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
 * name  = name within a section (e.g. "nb_ants", "simulate_ddos", etc.)
 * value = string found in .ini
 * Returns 1 on success, 0 if a parse error requires stopping.
 */
static int config_ini_handler(void* user, const char* section,
                              const char* name, const char* value)
{
    if (!user || !section || !name || !value) {
        return 0; /* invalid pointer => parse error */
    }

    AppConfig* cfg = (AppConfig*)user;

    /* [simulation] */
    if (strcmp(section, "simulation") == 0) {
        if      (strcmp(name, "nb_ants")      == 0) { cfg->nb_ants      = atoi(value); }
        else if (strcmp(name, "set_nb_nodes") == 0) { cfg->set_nb_nodes = atoi(value); }
        else if (strcmp(name, "min_hops")     == 0) { cfg->min_hops     = atoi(value); }
        else if (strcmp(name, "max_hops")     == 0) { cfg->max_hops     = atoi(value); }
    }
    /* [node] */
    else if (strcmp(section, "node") == 0) {
        if      (strcmp(name, "default_min_delay") == 0) { cfg->default_min_delay = atoi(value); }
        else if (strcmp(name, "default_max_delay") == 0) { cfg->default_max_delay = atoi(value); }
        else if (strcmp(name, "death_delay")       == 0) { cfg->death_delay       = atoi(value); }
        else if (strcmp(name, "under_attack_id")   == 0) { cfg->under_attack_id   = atoi(value); }
        else if (strcmp(name, "attack_started")    == 0) { cfg->attack_started    = parse_bool_value(value); }
    }
    /* [features] */
    else if (strcmp(section, "features") == 0) {
        if      (strcmp(name, "simulate_ddos")          == 0) { cfg->simulate_ddos          = parse_bool_value(value); }
        else if (strcmp(name, "show_random_performance")== 0) { cfg->show_random_performance= parse_bool_value(value); }
        else if (strcmp(name, "show_brute_performance") == 0) { cfg->show_brute_performance = parse_bool_value(value); }
    }
    /* [ranking] */
    else if (strcmp(section, "ranking") == 0) {
        if      (strcmp(name, "ranking_alpha") == 0) { cfg->ranking_alpha = atof(value); }
        else if (strcmp(name, "ranking_beta")  == 0) { cfg->ranking_beta  = atof(value); }
        else if (strcmp(name, "ranking_gamma") == 0) { cfg->ranking_gamma = atof(value); }
    }
    /* [ants] */
    else if (strcmp(section, "ants") == 0) {
        if      (strcmp(name, "ant_alpha")    == 0) { cfg->ant_alpha       = (float)atof(value); }
        else if (strcmp(name, "ant_beta")     == 0) { cfg->ant_beta        = (float)atof(value); }
        else if (strcmp(name, "Q")            == 0) { cfg->ant_Q           = (float)atof(value); }
        else if (strcmp(name, "evaporation")  == 0) { cfg->ant_evaporation = (float)atof(value); }
    }

    return 1; /* continue parsing */
}

/*
 * pub_config_load
 * Loads the .ini file at filepath into *cfg.
 * Returns true on success, false on error (file open or parse failure).
 * This function does not discard existing cfg fields if missing from the file.
 * Default values can be set beforehand with pub_config_set_defaults if needed.
 */
bool pub_config_load(AppConfig* cfg, const char* filepath)
{
    if (!cfg || !filepath) {
        return false;
    }

#ifndef _WIN32
    pthread_mutex_lock(&g_config_mutex);
#endif

    int parse_result = ini_parse(filepath, config_ini_handler, cfg);
    if (parse_result != 0) {
#ifndef _WIN32
        pthread_mutex_unlock(&g_config_mutex);
#endif
        return false;
    }

#ifndef _WIN32
    pthread_mutex_unlock(&g_config_mutex);
#endif
    return true;
}

/*
 * pub_config_save
 * Writes fields from *cfg to the .ini file at filepath.
 * Returns true on success, false otherwise.
 * Overwrites existing file contents.
 */
bool pub_config_save(const AppConfig* cfg, const char* filepath)
{
    if (!cfg || !filepath) {
        return false;
    }

#ifndef _WIN32
    pthread_mutex_lock(&g_config_mutex);
#endif

    FILE* fp = fopen(filepath, "w");
    if (!fp) {
#ifndef _WIN32
        pthread_mutex_unlock(&g_config_mutex);
#endif
        return false;
    }

    /* [simulation] */
    fprintf(fp, "[simulation]\n");
    fprintf(fp, "nb_ants = %d\n",         cfg->nb_ants);
    fprintf(fp, "set_nb_nodes = %d\n",    cfg->set_nb_nodes);
    fprintf(fp, "min_hops = %d\n",        cfg->min_hops);
    fprintf(fp, "max_hops = %d\n",        cfg->max_hops);

    /* [node] */
    fprintf(fp, "\n[node]\n");
    fprintf(fp, "default_min_delay = %d\n", cfg->default_min_delay);
    fprintf(fp, "default_max_delay = %d\n", cfg->default_max_delay);
    fprintf(fp, "death_delay = %d\n",       cfg->death_delay);
    fprintf(fp, "under_attack_id = %d\n",   cfg->under_attack_id);
    fprintf(fp, "attack_started = %s\n",    cfg->attack_started ? "true" : "false");

    /* [features] */
    fprintf(fp, "\n[features]\n");
    fprintf(fp, "simulate_ddos = %s\n",          cfg->simulate_ddos ? "true" : "false");
    fprintf(fp, "show_random_performance = %s\n",cfg->show_random_performance ? "true" : "false");
    fprintf(fp, "show_brute_performance = %s\n", cfg->show_brute_performance  ? "true" : "false");

    /* [ranking] */
    fprintf(fp, "\n[ranking]\n");
    fprintf(fp, "ranking_alpha = %f\n", cfg->ranking_alpha);
    fprintf(fp, "ranking_beta = %f\n",  cfg->ranking_beta);
    fprintf(fp, "ranking_gamma = %f\n", cfg->ranking_gamma);

    /* [ants] */
    fprintf(fp, "\n[ants]\n");
    fprintf(fp, "ant_alpha = %f\n",    cfg->ant_alpha);
    fprintf(fp, "ant_beta = %f\n",     cfg->ant_beta);
    fprintf(fp, "Q = %f\n",            cfg->ant_Q);
    fprintf(fp, "evaporation = %f\n",  cfg->ant_evaporation);

    fclose(fp);

#ifndef _WIN32
    pthread_mutex_unlock(&g_config_mutex);
#endif
    return true;
}
