/* src/c/backend.c */
/*
 * The relevant changes are in antnet_run_all_solvers to incorporate ACO logic
 * plus the newly added antnet_get_config for reading the AppConfig.
 * The rest of the file remains the same, except for new lines with aco_v1 calls.
 */

 #include "../../include/backend.h"
 #include "../../include/backend_topology.h"
 #include "../../include/error_codes.h"
 #include "../../include/cpu_ACOv1.h"
 #include "../../include/random_algo.h"
 #include "../../include/cpu_brute_force.h"
 #include "../../include/config_manager.h"
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 
 #ifndef _WIN32
 #include <pthread.h>
 #endif
 
 #define MAX_CONTEXTS 16
 
 static AntNetContext g_contexts[MAX_CONTEXTS];
 static int g_context_in_use[MAX_CONTEXTS] = {0};
 
 #ifndef _WIN32
 static pthread_mutex_t g_contexts_lock = PTHREAD_MUTEX_INITIALIZER;
 #endif
 
 AntNetContext* get_context_by_id(int context_id)
 {
     if (context_id < 0 || context_id >= MAX_CONTEXTS) {
         return NULL;
     }
 #ifndef _WIN32
     pthread_mutex_lock(&g_contexts_lock);
 #endif
     int in_use = g_context_in_use[context_id];
 #ifndef _WIN32
     pthread_mutex_unlock(&g_contexts_lock);
 #endif
 
     return in_use ? &g_contexts[context_id] : NULL;
 }
 
 int antnet_initialize(int node_count, int min_hops, int max_hops)
 {
 #ifndef _WIN32
     pthread_mutex_lock(&g_contexts_lock);
 #endif
     for (int i = 0; i < MAX_CONTEXTS; i++) {
         if (!g_context_in_use[i]) {
             g_context_in_use[i] = 1;
 #ifndef _WIN32
             pthread_mutex_unlock(&g_contexts_lock);
 #endif
             AntNetContext* ctx = &g_contexts[i];
             ctx->node_count = node_count;
             ctx->min_hops = min_hops;
             ctx->max_hops = max_hops;
             ctx->iteration = 0;
             ctx->nodes = NULL;
             ctx->edges = NULL;
             ctx->num_nodes = 0;
             ctx->num_edges = 0;
 
             ctx->random_best_length = 0;
             ctx->random_best_latency = 0;
             memset(ctx->random_best_nodes, 0, sizeof(ctx->random_best_nodes));
 
             config_set_defaults(&ctx->config);
             ctx->config.set_nb_nodes = node_count;
             ctx->config.min_hops = min_hops;
             ctx->config.max_hops = max_hops;
 
             ctx->brute_best_length = 0;
             ctx->brute_best_latency = 0;
             memset(ctx->brute_best_nodes, 0, sizeof(ctx->brute_best_nodes));
             memset(&ctx->brute_state, 0, sizeof(ctx->brute_state));
 
             /* ACO initialization fields */
             ctx->aco_best_length = 0;
             ctx->aco_best_latency = 0;
             memset(ctx->aco_best_nodes, 0, sizeof(ctx->aco_best_nodes));
             memset(&ctx->aco_v1, 0, sizeof(ctx->aco_v1));
 
 #ifndef _WIN32
             pthread_mutex_init(&ctx->lock, NULL);
 #endif
             return i;
         }
     }
 #ifndef _WIN32
     pthread_mutex_unlock(&g_contexts_lock);
 #endif
     return ERR_NO_FREE_SLOT;
 }
 
 int antnet_run_iteration(int context_id)
 {
     AntNetContext* ctx = get_context_by_id(context_id);
     if (!ctx) return ERR_INVALID_CONTEXT;
 #ifndef _WIN32
     pthread_mutex_lock(&ctx->lock);
 #endif
     ctx->iteration++;
 #ifndef _WIN32
     pthread_mutex_unlock(&ctx->lock);
 #endif
     return ERR_SUCCESS;
 }
 
 int antnet_get_best_path(
     int context_id,
     int* out_nodes,
     int max_size,
     int* out_path_len,
     int* out_total_latency
 )
 {
     if (!out_nodes || !out_path_len || !out_total_latency) {
         return ERR_INVALID_ARGS;
     }
     AntNetContext* ctx = get_context_by_id(context_id);
     if (!ctx) return ERR_INVALID_CONTEXT;
 
 #ifndef _WIN32
     pthread_mutex_lock(&ctx->lock);
 #endif
     if (ctx->random_best_length > 0) {
         if (ctx->random_best_length > max_size) {
 #ifndef _WIN32
             pthread_mutex_unlock(&ctx->lock);
 #endif
             return ERR_ARRAY_TOO_SMALL;
         }
         memcpy(out_nodes, ctx->random_best_nodes, ctx->random_best_length * sizeof(int));
         *out_path_len = ctx->random_best_length;
         *out_total_latency = ctx->random_best_latency;
     } else {
         static int mock_nodes[] = {1, 2, 3, 5, 7, 9};
         int length = (int)(sizeof(mock_nodes) / sizeof(mock_nodes[0]));
         if (length > max_size) {
 #ifndef _WIN32
             pthread_mutex_unlock(&ctx->lock);
 #endif
             return ERR_ARRAY_TOO_SMALL;
         }
         memcpy(out_nodes, mock_nodes, length * sizeof(mock_nodes[0]));
         *out_path_len = length;
         *out_total_latency = 42 + ctx->iteration;
     }
 #ifndef _WIN32
     pthread_mutex_unlock(&ctx->lock);
 #endif
     return ERR_SUCCESS;
 }
 
 int antnet_shutdown(int context_id)
 {
     AntNetContext* ctx = get_context_by_id(context_id);
     if (!ctx) return ERR_INVALID_CONTEXT;
 
 #ifndef _WIN32
     pthread_mutex_lock(&ctx->lock);
 #endif
     if (ctx->nodes) {
         free(ctx->nodes);
         ctx->nodes = NULL;
     }
     if (ctx->edges) {
         free(ctx->edges);
         ctx->edges = NULL;
     }
     /* ACO arrays */
     if (ctx->aco_v1.adjacency) {
         free(ctx->aco_v1.adjacency);
         ctx->aco_v1.adjacency = NULL;
     }
     if (ctx->aco_v1.pheromones) {
         free(ctx->aco_v1.pheromones);
         ctx->aco_v1.pheromones = NULL;
     }
     printf("[antnet_shutdown] context %d final iteration: %d\n", context_id, ctx->iteration);
 #ifndef _WIN32
     pthread_mutex_unlock(&ctx->lock);
     pthread_mutex_destroy(&ctx->lock);
 #endif
 
 #ifndef _WIN32
     pthread_mutex_lock(&g_contexts_lock);
 #endif
     g_context_in_use[context_id] = 0;
 #ifndef _WIN32
     pthread_mutex_unlock(&g_contexts_lock);
 #endif
     return ERR_SUCCESS;
 }
 
 int antnet_run_all_solvers(
     int context_id,
     int* out_nodes_aco,
     int max_size_aco,
     int* out_len_aco,
     int* out_latency_aco,
     int* out_nodes_random,
     int max_size_random,
     int* out_len_random,
     int* out_latency_random,
     int* out_nodes_brute,
     int max_size_brute,
     int* out_len_brute,
     int* out_latency_brute
 )
 {
     if (!out_nodes_aco || !out_len_aco || !out_latency_aco ||
         !out_nodes_random || !out_len_random || !out_latency_random ||
         !out_nodes_brute || !out_len_brute || !out_latency_brute) {
         return ERR_INVALID_ARGS;
     }
 
     AntNetContext* ctx = get_context_by_id(context_id);
     if (!ctx) return ERR_INVALID_CONTEXT;
 
 #ifndef _WIN32
     pthread_mutex_lock(&ctx->lock);
 #endif
     *out_len_aco = 0;
     *out_latency_aco = 0;
 
     /* 1) ACO */
     int rc = aco_v1_run_iteration(ctx);
     if (rc != ERR_SUCCESS && rc != ERR_NO_TOPOLOGY) {
 #ifndef _WIN32
         pthread_mutex_unlock(&ctx->lock);
 #endif
         return rc;
     }
     rc = aco_v1_search_path(ctx, 0, 1,
                             out_nodes_aco, max_size_aco,
                             out_len_aco, out_latency_aco);
     if (rc != ERR_SUCCESS && rc != ERR_NO_PATH_FOUND) {
 #ifndef _WIN32
         pthread_mutex_unlock(&ctx->lock);
 #endif
         return rc;
     }
 
     /* 2) RANDOM */
     rc = random_search_path(
         ctx, 0, 1,
         out_nodes_random, max_size_random,
         out_len_random, out_latency_random
     );
     if (rc != ERR_SUCCESS && rc != ERR_NO_PATH_FOUND) {
 #ifndef _WIN32
         pthread_mutex_unlock(&ctx->lock);
 #endif
         return rc;
     }
 
     /* 3) BRUTE */
     rc = brute_force_search_step(
         ctx, 0, 1,
         out_nodes_brute, max_size_brute,
         out_len_brute, out_latency_brute
     );
     if (rc != ERR_SUCCESS && rc != ERR_NO_PATH_FOUND) {
 #ifndef _WIN32
         pthread_mutex_unlock(&ctx->lock);
 #endif
         return rc;
     }
 
 #ifndef _WIN32
     pthread_mutex_unlock(&ctx->lock);
 #endif
     return ERR_SUCCESS;
 }
 
 int antnet_init_from_config(const char* config_path)
 {
     if (!config_path) return ERR_INVALID_ARGS;
     AppConfig tmpcfg;
     config_set_defaults(&tmpcfg);
 
     if (!config_load(&tmpcfg, config_path)) {
         return -1;
     }
     int context_id = antnet_initialize(
         tmpcfg.set_nb_nodes,
         tmpcfg.min_hops,
         tmpcfg.max_hops
     );
     if (context_id < 0) return context_id;
 
     AntNetContext* ctx = get_context_by_id(context_id);
     if (!ctx) return ERR_INVALID_CONTEXT;
 
 #ifndef _WIN32
     pthread_mutex_lock(&ctx->lock);
 #endif
     ctx->config = tmpcfg;
 #ifndef _WIN32
     pthread_mutex_unlock(&ctx->lock);
 #endif
     return context_id;
 }
 
 /*
  * antnet_get_config: thread-safe read of the current context config
  */
 int antnet_get_config(int context_id, AppConfig* out)
 {
     if (!out) return ERR_INVALID_ARGS;
 
     AntNetContext* ctx = get_context_by_id(context_id);
     if (!ctx) return ERR_INVALID_CONTEXT;
 
 #ifndef _WIN32
     pthread_mutex_lock(&ctx->lock);
 #endif
     *out = ctx->config;
 #ifndef _WIN32
     pthread_mutex_unlock(&ctx->lock);
 #endif
 
     return ERR_SUCCESS;
 }
 
 /*
  * antnet_get_pheromone_matrix: returns the entire pheromone matrix, thread-safe.
  * It copies up to n*n floats into 'out', where n = ctx->aco_v1.pheromone_size.
  * Returns (n*n) on success, or negative error code on failure.
  */
 int antnet_get_pheromone_matrix(int context_id, float* out, int max_count)
 {
     AntNetContext* ctx = get_context_by_id(context_id);
     if (!ctx) return ERR_INVALID_CONTEXT;
 
 #ifndef _WIN32
     pthread_mutex_lock(&ctx->lock);
 #endif
     if (!ctx->aco_v1.pheromones || ctx->aco_v1.pheromone_size <= 0) {
 #ifndef _WIN32
         pthread_mutex_unlock(&ctx->lock);
 #endif
         return ERR_NO_TOPOLOGY;
     }
     int n = ctx->aco_v1.pheromone_size;
     int count = n * n;
     if (max_count < count) {
 #ifndef _WIN32
         pthread_mutex_unlock(&ctx->lock);
 #endif
         return ERR_ARRAY_TOO_SMALL;
     }
     memcpy(out, ctx->aco_v1.pheromones, sizeof(float) * count);
 
 #ifndef _WIN32
     pthread_mutex_unlock(&ctx->lock);
 #endif
     return count;
 }
 