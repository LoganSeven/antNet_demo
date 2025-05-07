//src/c/cpu_brute_force.c

/*
 * cpu_brute_force.c
 * Implements a depth-first search to examine all feasible paths between start_id and end_id
 * within the hop limits, then returns the path of minimal total latency.
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <limits.h>
 #include "cpu_brute_force.h"
 #include "error_codes.h"
 
 /*
  * Use a backtracking approach. For real usage in large graphs, this might be expensive,
  * but it guarantees the best path if the graph is not too large or the hops are restricted.
  */
 
 static int best_latency_global = INT_MAX;
 static int best_path_nodes[1024];
 static int best_path_len_global = 0;
 
 /*
  * helper function: gather neighbors
  */
 static void get_neighbors_brute(const AntNetContext* ctx, int node_id, int* out_neighbors, int* neighbor_count, int max_count)
 {
     int count = 0;
     for (int i = 0; i < ctx->num_edges; i++) {
         if (ctx->edges[i].from_id == node_id) {
             if (count < max_count) {
                 out_neighbors[count] = ctx->edges[i].to_id;
                 count++;
             }
         }
     }
     *neighbor_count = count;
 }
 
 /*
  * recursive DFS
  */
 static void dfs_brute(
     const AntNetContext* ctx,
     int current_id,
     int end_id,
     int* path,
     int path_len,
     int* visited,
     int current_latency
 )
 {
     // If path_len exceeds max_hops, stop exploring
     if (path_len > ctx->max_hops) {
         return;
     }
 
     // If reached end_id, check min_hops and update best if valid
     if (current_id == end_id && path_len >= ctx->min_hops) {
         if (current_latency < best_latency_global) {
             best_latency_global = current_latency;
             best_path_len_global = path_len;
             for (int i = 0; i < path_len; i++) {
                 best_path_nodes[i] = path[i];
             }
         }
         return;
     }
 
     // Gather neighbors
     int neighbors[1024];
     int neighbor_count = 0;
     get_neighbors_brute(ctx, current_id, neighbors, &neighbor_count, 1024);
 
     for (int i = 0; i < neighbor_count; i++) {
         int nid = neighbors[i];
         // same assumption as random_algo: node_id < ctx->num_nodes
         if (nid >= 0 && nid < ctx->num_nodes && !visited[nid]) {
             visited[nid] = 1;
             path[path_len] = nid;
             int new_latency = current_latency + ctx->nodes[nid].delay_ms;
             dfs_brute(ctx, nid, end_id, path, path_len + 1, visited, new_latency);
             visited[nid] = 0;
         }
     }
 }
 
 int brute_force_path(
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
         return ERR_INVALID_ARGS ;
     }
     if (ctx->num_nodes <= 0 || ctx->num_edges <= 0) {
         return ERR_NO_TOPOLOGY;
     }
 
     // Global variables for best solution
     best_latency_global = INT_MAX;
     best_path_len_global = 0;
 
     int visited[1024] = {0};
     int path[1024];
 
     // Initialize path
     path[0] = start_id;
     visited[start_id] = 1;
     int initial_latency = 0;
     if (start_id >= 0 && start_id < ctx->num_nodes) {
         initial_latency = ctx->nodes[start_id].delay_ms;
     }
 
     dfs_brute(ctx, start_id, end_id, path, 1, visited, initial_latency);
 
     if (best_latency_global < INT_MAX && best_path_len_global > 0) {
         if (best_path_len_global <= max_size) {
             for (int i = 0; i < best_path_len_global; i++) {
                 out_nodes[i] = best_path_nodes[i];
             }
             *out_path_len = best_path_len_global;
             *out_total_latency = best_latency_global;
             return ERR_SUCCESS; // success
         } else {
             // user-provided array too small
             return ERR_ARRAY_TOO_SMALL;
         }
     }
 
     // no valid path found
     return ERR_NO_PATH_FOUND;
 }
