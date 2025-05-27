/* Relative Path: src/c/managers/cpu_acoV1_algo_manager.c */
/*
 * Manages the ACO V1 algorithm, coordinating initialization, iteration, and path search.
 * Provides a unified interface to the underlying cpu_ACOv1 implementation.
 * Useful for hooking ACO steps into the main AntNet flow.
*/


#include "../../../include/rendering/heatmap_renderer_api.h"
#include "../../../include/managers/cpu_acoV1_algo_manager.h"
#include "../../../include/algo/cpu/cpu_ACOv1.h"


/* aco_algo_manager_init
 * Initializes the ACO solver (if needed).
 */
int aco_algo_manager_init(AntNetContext* ctx)
{
    if (!ctx)
    {
        return ERR_INVALID_CONTEXT;
    }
    /* No specific initialization required for ACO at this time. */
    return ERR_SUCCESS;
}

/* aco_algo_manager_run_iteration
 * Performs one iteration of the ACO algorithm.
 */
int aco_algo_manager_run_iteration(AntNetContext* ctx)
{
    if (!ctx)
    {
        return ERR_INVALID_CONTEXT;
    }
    return aco_v1_run_iteration(ctx);
}

/* aco_algo_manager_search_path
 * Uses ACO data to find a path between start_node and end_node.
 */
int aco_algo_manager_search_path(
    AntNetContext* ctx,
    int start_node,
    int end_node,
    int* out_nodes,
    int max_size,
    int* out_length,
    int* out_latency
)
{
    if (!ctx)
    {
        return ERR_INVALID_CONTEXT;
    }
    return aco_v1_search_path(
        ctx,
        start_node,
        end_node,
        out_nodes,
        max_size,
        out_length,
        out_latency
    );
}

/* aco_algo_manager_cleanup
 * Cleans up any allocated resources for the ACO solver (if needed).
 */
int aco_algo_manager_cleanup(AntNetContext* ctx)
{
    if (!ctx)
    {
        return ERR_INVALID_CONTEXT;
    }
    /* No specific cleanup required at this time. */
    return ERR_SUCCESS;
}
