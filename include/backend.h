#ifndef ANTNET_BACKEND_H
#define ANTNET_BACKEND_H

#ifdef __cplusplus
extern "C" {
#endif

// Backend context handle
typedef struct AntNetContext AntNetContext;

// Structured path info
typedef struct {
    int* nodes;        // pointer to an array of node indices
    int  node_count;   // number of nodes in the path
    int  total_latency;// total latency in ms
} AntNetPathInfo;

// Create and initialize a simulation context
AntNetContext* antnet_init(int node_count, int min_hops, int max_hops);

// Run one iteration of the simulation
void antnet_run_iteration(AntNetContext* ctx);

// Get best path found so far (mocked)
// Returns pointer to a static AntNetPathInfo
const AntNetPathInfo* antnet_get_best_path_struct(AntNetContext* ctx);

// Clean up and free the context
void antnet_shutdown(AntNetContext* ctx);

#ifdef __cplusplus
}
#endif

#endif // ANTNET_BACKEND_H
