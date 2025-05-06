/*
 * random_algo.c
 * Implements a random path finder that uses the node/edge data from AntNetContext.
 * This does not guarantee an optimal route, only a feasible route within hop constraints.
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <time.h>
 #include "random_algo.h"
 
 /*
  * Helper: returns a list of neighbor node_ids for a given node_id,
  * scanning the ctx->edges array for all edges that match.
  * The out_neighbors array must be allocated by the caller; neighbor_count is set on return.
  */
 static void get_neighbors(const AntNetContext* ctx, int node_id, int* out_neighbors, int* neighbor_count, int max_possible)
 {
     int count = 0;
     for (int i = 0; i < ctx->num_edges; i++) {
         if (ctx->edges[i].from_id == node_id) {
             // Potential neighbor
             if (count < max_possible) {
                 out_neighbors[count] = ctx->edges[i].to_id;
                 count++;
             }
         }
     }
     *neighbor_count = count;
 }
 
 /*
  * random_search_path: main function
  * Note: This implementation tries a fixed number of random attempts
  * to find a path. If it fails, returns negative.
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
         return -1;
     }
 
     // A simple random-based approach with a limited number of tries
     // to avoid infinite loops in large graphs.
     const int MAX_ATTEMPTS = 200;
 
     srand((unsigned int)time(NULL));  // randomize seed once per call
 
     // If there is no valid node array or edge array, return
     if (ctx->num_nodes <= 0 || ctx->num_edges <= 0) {
         return -2;
     }
 
     int best_path_len = 0;
     int best_latency = 0;
     int found_path = 0;
 
     // Attempt multiple random walks
     for (int attempt = 0; attempt < MAX_ATTEMPTS; attempt++) {
         int path[1024]; // local scratch
         int visited[1024] = {0};
         int path_len = 0;
         int total_latency = 0;
 
         if (max_size > (int)(sizeof(path)/sizeof(path[0]))) {
             // limit max_size if user gave something huge
             max_size = (int)(sizeof(path)/sizeof(path[0]));
         }
 
         // Start at start_id
         path[0] = start_id;
         visited[start_id] = 1;
         total_latency += ctx->nodes[0].delay_ms; // This is simplistic, see below for indexing
         path_len = 1;
 
         int current_id = start_id;
         while (current_id != end_id) {
             if (path_len >= ctx->max_hops) {
                 // Exceeded max hops
                 break;
             }
             // Gather neighbors
             int neighbors[1024];
             int neighbor_count = 0;
             get_neighbors(ctx, current_id, neighbors, &neighbor_count, 1024);
 
             // Filter out visited
             int unvisited[1024];
             int uv_count = 0;
             for (int i = 0; i < neighbor_count; i++) {
                 int nid = neighbors[i];
                 // Because node_id could be arbitrary, map them into the NodeData array
                 // This approach assumes node_id < ctx->num_nodes, or a direct match
                 // If real IDs are scattered, a map might be needed.
                 // For simplicity, assume node_id is in [0..num_nodes-1].
                 if (nid >= 0 && nid < ctx->num_nodes && visited[nid] == 0) {
                     unvisited[uv_count] = nid;
                     uv_count++;
                 }
             }
 
             if (uv_count == 0) {
                 // No unvisited neighbors
                 break;
             }
 
             // Choose randomly among unvisited
             int next_index = rand() % uv_count;
             int next_id = unvisited[next_index];
             path[path_len] = next_id;
             visited[next_id] = 1;
             path_len++;
 
             // Add latency (assuming node_id matches index)
             total_latency += ctx->nodes[next_id].delay_ms;
 
             current_id = next_id;
 
             // If we reached end_id, check if path_len >= min_hops
             if (current_id == end_id && path_len >= ctx->min_hops) {
                 found_path = 1;
                 if (best_path_len == 0 || total_latency < best_latency) {
                     // Found a better random path
                     best_path_len = path_len;
                     best_latency = total_latency;
                     // Copy to user array
                     if (best_path_len <= max_size) {
                         for (int k = 0; k < best_path_len; k++) {
                             out_nodes[k] = path[k];
                         }
                         *out_path_len = best_path_len;
                         *out_total_latency = best_latency;
                     }
                 }
                 break;
             }
         }
     }
 
     if (found_path && best_path_len <= max_size) {
         return 0; // success
     }
 
     // Could not find a path meeting constraints
     return -3;
 }
