//src/c/random_algo.c
/*
 * random_algo.c
 * Implements a random path finder that uses the node/edge data from AntNetContext.
 * This version picks a path length in [min_hops..max_hops], randomly chooses that many
 * distinct nodes (excluding start and end), builds a path start -> chosen_nodes -> end,
 * sums latency, and updates ctx->random_best_* fields if a better solution is found.
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <time.h>
 #include "../../include/error_codes.h"
 #include "../../include/random_algo.h"
 
 /*
  * The commented code below is kept for future usage; it is not removed.
  * #if 0
  *  Old random-walk approach
  *  ...
  * #endif
  */
 
 /*
  * random_search_path: main function
  * - picks a random count in [ctx->min_hops..ctx->max_hops]
  * - selects distinct random nodes from the range [2..ctx->num_nodes-1] if possible
  * - forms a path (start_id + chosen + end_id)
  * - sums total latency from each node in that path
  * - if it improves the stored best path in ctx->random_best_*, updates it
  * - copies the best path so far into out_nodes, out_path_len, out_total_latency
  * Returns 0 on success, negative error codes otherwise
  */
 int random_search_path(
     AntNetContext* ctx,
     int start_id,
     int end_id,
     int* out_nodes,
     int max_size,
     int* out_path_len,
     int* out_total_latency
 )
 {
     if (!ctx || !out_nodes || !out_path_len || !out_total_latency) {
         return ERR_INVALID_ARGS;
     }
     if (ctx->num_nodes <= 0 || ctx->nodes == NULL) {
         return ERR_NO_TOPOLOGY;
     }
     // Acquire lock for thread safety
 #ifndef _WIN32
     pthread_mutex_lock(&ctx->lock);
 #endif
 
     // Ensure the result array is large enough for worst case (max_hops + 2)
     // For safety, set an upper bound
     int needed_capacity = ctx->max_hops + 2; 
     if (needed_capacity > max_size || needed_capacity > 1024) {
 #ifndef _WIN32
         pthread_mutex_unlock(&ctx->lock);
 #endif
         return ERR_ARRAY_TOO_SMALL;
     }
 
     // Random seed once per call
     srand((unsigned int)time(NULL));
 
     // 1) pick random number of selected hops
     int range_size = ctx->max_hops - ctx->min_hops + 1;
     if (range_size <= 0) {
 #ifndef _WIN32
         pthread_mutex_unlock(&ctx->lock);
 #endif
         return ERR_INVALID_ARGS;
     }
     int nb_selected_nodes = ctx->min_hops + (rand() % range_size);
 
     // 2) build a list of candidate node IDs (exclude start_id=0, end_id=1)
     //    then shuffle and pick nb_selected_nodes
     int candidate_count = ctx->num_nodes - 2; // excluding 0 and 1
     if (candidate_count < 0) {
 #ifndef _WIN32
         pthread_mutex_unlock(&ctx->lock);
 #endif
         return ERR_NO_PATH_FOUND;
     }
     // gather candidate node IDs
     int* candidates = (int*)malloc(candidate_count * sizeof(int));
     if (!candidates) {
 #ifndef _WIN32
         pthread_mutex_unlock(&ctx->lock);
 #endif
         return ERR_MEMORY_ALLOCATION;
     }
     int idx = 0;
     for (int i = 0; i < ctx->num_nodes; i++) {
         if (i != start_id && i != end_id) {
             candidates[idx++] = i;
         }
     }
     // if nb_selected_nodes > candidate_count, no feasible path
     if (nb_selected_nodes > candidate_count) {
         free(candidates);
 #ifndef _WIN32
         pthread_mutex_unlock(&ctx->lock);
 #endif
         return ERR_NO_PATH_FOUND;
     }
 
     // Fisher-Yates shuffle
     for (int i = candidate_count - 1; i > 0; i--) {
         int j = rand() % (i + 1);
         int tmp = candidates[i];
         candidates[i] = candidates[j];
         candidates[j] = tmp;
     }
 
     // pick the first nb_selected_nodes from shuffled array
     // and optionally shuffle them again if a random order is desired in the path
     // for simplicity, just keep them in the order we got from the shuffle
     int new_path_length = nb_selected_nodes + 2; // including start, end
     int* new_path = (int*)malloc(new_path_length * sizeof(int));
     if (!new_path) {
         free(candidates);
 #ifndef _WIN32
         pthread_mutex_unlock(&ctx->lock);
 #endif
         return ERR_MEMORY_ALLOCATION;
     }
 
     new_path[0] = start_id;
     for (int k = 0; k < nb_selected_nodes; k++) {
         new_path[k + 1] = candidates[k];
     }
     new_path[new_path_length - 1] = end_id;
 
     // 3) compute total latency
     // The array ctx->nodes[] is assumed indexed by node_id
     int new_total_latency = 0;
     for (int k = 0; k < new_path_length; k++) {
         int node_id = new_path[k];
         if (node_id < 0 || node_id >= ctx->num_nodes) {
             // invalid node
             free(new_path);
             free(candidates);
 #ifndef _WIN32
             pthread_mutex_unlock(&ctx->lock);
 #endif
             return ERR_NO_PATH_FOUND;
         }
         new_total_latency += ctx->nodes[node_id].delay_ms;
     }
 
     // 4) if it is better or if no best path is stored yet, update ctx->random_best_*
     // The default for random_best_length is 0 => no path set yet
     if (ctx->random_best_length == 0 || new_total_latency < ctx->random_best_latency) {
         ctx->random_best_length = new_path_length;
         ctx->random_best_latency = new_total_latency;
         for (int p = 0; p < new_path_length; p++) {
             ctx->random_best_nodes[p] = new_path[p];
         }
     }
 
     // free temporary arrays
     free(new_path);
     free(candidates);
 
     // Copy the best path so far to out_* fields
     if (ctx->random_best_length > max_size) {
 #ifndef _WIN32
         pthread_mutex_unlock(&ctx->lock);
 #endif
         return ERR_ARRAY_TOO_SMALL;
     }
     for (int p = 0; p < ctx->random_best_length; p++) {
         out_nodes[p] = ctx->random_best_nodes[p];
     }
     *out_path_len = ctx->random_best_length;
     *out_total_latency = ctx->random_best_latency;
 
 #ifndef _WIN32
     pthread_mutex_unlock(&ctx->lock);
 #endif
 
     return ERR_SUCCESS;
 }
 
