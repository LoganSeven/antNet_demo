/* include/backend.h */
/*
 * backend.h
 * Main public API for the AntNet backend.
 * (Original content preserved; only adapted to add ACO fields and get_config.)
 */

 #ifndef BACKEND_H
 #define BACKEND_H
 
 #include "../types/antnet_config_types.h"
 #include "../types/antnet_brute_force_types.h"
 #include "../types/antnet_path_types.h"
 #include "../types/antnet_network_types.h"
 #include "../consts/error_codes.h"
 #include "../backend_thread_defs.h"
 #include "../types/antnet_aco_v1_types.h"
 #include "../rendering/heatmap_renderer.h"
 #include "../rendering/heatmap_renderer_async.h" /* added for async approach */
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 /*
  * AntNetContext: stores the ACA context, including nodes, edges,
  * and algorithm-specific parameters.  The structure is visible to CFFI.
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
 
     /* internal brute force state */
     BruteForceState brute_state;
 
     /* aco solver best path */
     int  aco_best_nodes[1024];
     int  aco_best_length;
     int  aco_best_latency;
 
     /* internal ACO v1 solver state */
     AcoV1State aco_v1;
 
 } AntNetContext;
 
 /* public API */
 
 int antnet_initialize(int node_count, int min_hops, int max_hops);
 int antnet_run_iteration(int context_id);
 int antnet_shutdown(int context_id);
 
 int antnet_get_best_path(
     int  context_id,
     int *out_nodes,
     int  max_size,
     int *out_path_len,
     int *out_total_latency
 );
 
 int antnet_run_all_solvers(
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
 
 int antnet_init_from_config(const char *config_path);
 
 /*
  * antnet_get_config: thread-safe read of the current context config
  */
 int antnet_get_config(int context_id, AppConfig* out);
 
 /*
  * antnet_get_pheromone_matrix: thread-safe retrieval of the entire pheromone
  * matrix of size n*n, where n = ctx->aco_v1.pheromone_size. Writes up to
  * max_count floats into 'out'. Returns the number of floats (n*n) on success,
  * or negative on error.
  */
 int antnet_get_pheromone_matrix(int context_id, float* out, int max_count);
 
 /*
  * antnet_render_heatmap_rgba
  *
  * Renders a heatmap from the given point cloud and pheromone values.
  * Uses the persistent background renderer in heatmap_renderer_async.c
  *
  * Parameters:
  *   pts_xy     - Array of n (x, y) float pairs in [-1, 1] space (2 * n floats total)
  *   strength   - Array of n pheromone values ∈ [0..1]
  *   n          - Number of points
  *   out_rgba   - Output buffer (width * height * 4) bytes, preallocated
  *   width      - Width of the output image
  *   height     - Height of the output image
  *
  * Returns:
  *   ERR_SUCCESS on success, or ERR_INTERNAL_FAILURE on error.
  */
 int antnet_render_heatmap_rgba(
     const float *pts_xy,
     const float *strength,
     int n,
     unsigned char *out_rgba,
     int width,
     int height
 );
 
 /*
  * antnet_renderer_async_init
  * Starts the persistent renderer thread. Must be called once on app startup.
  */
 int antnet_renderer_async_init(int initial_width, int initial_height);
 
 /*
  * antnet_renderer_async_shutdown
  * Stops the persistent renderer thread and cleans up.
  */
 int antnet_renderer_async_shutdown(void);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* BACKEND_H */
 