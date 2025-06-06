/* Relative Path: include/rendering/heatmap_renderer_api.h */
/* 
 * Main public API for initializing, running, and managing AntNet contexts.
 * Defines the AntNetContext struct and functions for solver coordination, config, and I/O.
 * Entry point for external modules interacting with the AntNet backend.
*/

#ifndef BACKEND_H
#define BACKEND_H

#include "../types/antnet_config_types.h"
#include "../types/antnet_brute_force_types.h"
#include "../types/antnet_path_types.h"
#include "../types/antnet_network_types.h"
#include "../consts/error_codes.h"
#include "../core/backend_thread_defs.h"
#include "../types/antnet_aco_v1_types.h"
#include "../rendering/heatmap_renderer.h"
#include "../rendering/heatmap_renderer_async.h"
#include "../managers/hop_map_manager.h"

/* NEW: include RankingEntry definition */
#include "../types/antnet_ranking_types.h"

/* NEW: include SasaCoeffs structure */
#include "../types/antnet_sasa_types.h"

/* Forward-declare HopMapManager so we can store a pointer to it. */
struct HopMapManager;

#ifdef __cplusplus
extern "C" {
#endif

/*
 * AntNetContext: stores the ACA context, including nodes, edges,
 * and algorithm-specific parameters. The structure is visible to CFFI.
 */
typedef struct AntNetContext
{
    /* basic topology parameters */
    int node_count;
    int min_hops;
    int max_hops;

    /* dynamic topology */
    NodeData *nodes;
    int       num_nodes;
    EdgeData *edges;
    int       num_edges;
    int       iteration;

    /* thread safety */
    pthread_mutex_t lock;

    /* random solver best path */
    int  random_best_nodes[1024];
    int  random_best_length;
    int  random_best_latency;

    /* configuration currently loaded */
    AppConfig config;

    /* brute force solver best path */
    int  brute_best_nodes[1024];
    int  brute_best_length;
    int  brute_best_latency;
    BruteForceState brute_state; /* internal brute force state */

    /* aco solver best path */
    int  aco_best_nodes[1024];
    int  aco_best_length;
    int  aco_best_latency;
    AcoV1State aco_v1; /* internal ACO v1 solver state */

    /* SASA addition: store the incremental scoring state for each solver */
    SasaState aco_sasa;
    SasaState random_sasa;
    SasaState brute_sasa;

    /* NEW: store SASA coefficients used in run_all_solvers, etc. */
    SasaCoeffs sasa_coeffs;

    /* NEW: pointer to the HopMapManager for node/edge arrangement. */
    HopMapManager *hop_map_mgr; /* Manages hop-based node layout inside this context */

} AntNetContext;

/* public API */

int pub_initialize(int node_count, int min_hops, int max_hops);
int pub_run_iteration(int context_id);
int pub_shutdown(int context_id);

int pub_get_best_path(
    int  context_id,
    int *out_nodes,
    int  max_size,
    int *out_path_len,
    int *out_total_latency
);

int pub_run_all_solvers(
    int  context_id,
    /* aco */
    int *out_nodes_aco,
    int  max_size_aco,
    int *out_len_aco,
    int *out_latency_aco,
    /* random */
    int *out_nodes_random,
    int  max_size_random,
    int *out_len_random,
    int *out_latency_random,
    /* brute */
    int *out_nodes_brute,
    int  max_size_brute,
    int *out_len_brute,
    int *out_latency_brute
);

int pub_init_from_config(const char *config_path);

/*
 * pub_get_config
 * Thread-safe read of the current context config.
 */
int pub_get_config(int context_id, AppConfig* out);

/*
 * pub_get_pheromone_matrix
 * Thread-safe retrieval of the entire pheromone matrix of size n*n,
 * where n = ctx->aco_v1.pheromone_size.
 * Writes up to max_count floats into 'out'. Returns the number of floats (n*n)
 * on success, or negative on error.
 */
int pub_get_pheromone_matrix(int context_id, float* out, int max_count);

/*
 * pub_render_heatmap_rgba
 *
 * Renders a heatmap from the given point cloud and pheromone values.
 * Uses the persistent background renderer in heatmap_renderer_async.c.
 */
int pub_render_heatmap_rgba(
    const float *pts_xy,
    const float *strength,
    int n,
    unsigned char *out_rgba,
    int width,
    int height
);

int pub_renderer_async_init(int initial_width, int initial_height);
int pub_renderer_async_shutdown(void);

/*
 * pub_get_algo_ranking
 * Returns the list of algorithms sorted by SASA score in descending order.
 * Writes up to max_count entries in out[]. Returns the actual count of
 * algorithms (e.g., 3) on success. If max_count < 3, returns a negative error.
 */
int pub_get_algo_ranking(int context_id, RankingEntry* out, int max_count);

/*
 * NEW: pub_set_sasa_params
 * Updates the SASA coefficients (alpha, beta, gamma) used by the solvers
 * to compute incremental SASA scoring. Thread-safe.
 */
int pub_set_sasa_params(int context_id, double alpha, double beta, double gamma);

/*
 * NEW: pub_get_sasa_params
 * Reads the SASA coefficients (alpha, beta, gamma) from the context. Thread-safe.
 */
int pub_get_sasa_params(int context_id, double* out_alpha, double* out_beta, double* out_gamma);

/*
 * NEW: pub_set_aco_params
 * Updates the main ACO parameters in the context (alpha, beta, Q, evaporation, num_ants).
 * Thread-safe.
 */
int pub_set_aco_params(int context_id, float alpha, float beta, float Q, float evaporation, int num_ants);

/*
 * NEW: pub_get_aco_params
 * Reads the ACO parameters (alpha, beta, Q, evaporation, num_ants). Thread-safe.
 */
int pub_get_aco_params(
    int context_id,
    float* out_alpha,
    float* out_beta,
    float* out_Q,
    float* out_evaporation,
    int*  out_num_ants
);


#ifdef __cplusplus
}
#endif

#endif /* BACKEND_H */
