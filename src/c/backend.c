#include "backend.h"
#include <stdio.h>
#include <stdlib.h>

// Define the context struct
struct AntNetContext {
    int node_count;
    int min_hops;
    int max_hops;
    int iteration;
};

// Static mock data for path info
static int mock_nodes[] = {1, 3, 7, 9};
static AntNetPathInfo static_path = {
    .nodes = mock_nodes,
    .node_count = sizeof(mock_nodes)/sizeof(mock_nodes[0]),
    .total_latency = 0
};

AntNetContext* antnet_init(int node_count, int min_hops, int max_hops) {
    AntNetContext* ctx = malloc(sizeof(AntNetContext));
    if (!ctx) return NULL;
    ctx->node_count = node_count;
    ctx->min_hops   = min_hops;
    ctx->max_hops   = max_hops;
    ctx->iteration  = 0;

    printf("[AntNet] Initialized context: %d nodes, hops [%d–%d]\n",
           node_count, min_hops, max_hops);
    return ctx;
}

void antnet_run_iteration(AntNetContext* ctx) {
    if (!ctx) return;
    ctx->iteration++;
    printf("[AntNet] Iteration %d executed (mocked).\n", ctx->iteration);
}

const AntNetPathInfo* antnet_get_best_path_struct(AntNetContext* ctx) {
    if (!ctx) return NULL;
    // Update latency mock
    static_path.total_latency = 42 + ctx->iteration;
    return &static_path;
}

void antnet_shutdown(AntNetContext* ctx) {
    if (!ctx) return;
    printf("[AntNet] Shutdown. Total iterations: %d\n", ctx->iteration);
    free(ctx);
}
