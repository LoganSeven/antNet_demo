/* include/backend.h */
/*
 * backend.h
 * Main public API for the AntNet backend.
 * (Original content preserved; only adapted to add ACO fields and get_config.)
 */

 #ifndef BACKEND_H
 #define BACKEND_H
 
 #include "antnet_config_types.h"
 #include "antnet_brute_force_types.h"
 #include "antnet_path_types.h"
 #include "antnet_network_types.h"
 #include "error_codes.h"
 #include "backend_thread_defs.h"
 #include "antnet_aco_v1_types.h"  /* newly added for AcoV1State */
 
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
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif /* BACKEND_H */
 