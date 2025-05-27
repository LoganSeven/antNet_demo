/* Relative Path: src/c/managers/cpu_random_algo_manager.c */
/*
 * Manages a simple random pathfinding solver for baseline comparisons.
 * Coordinates random search, capturing best path found and updating states.
 * Minimal overhead, useful for measuring performance against advanced solvers.
*/

#include "../../../include/rendering/heatmap_renderer_api.h"
#include "../../../include/managers/cpu_random_algo_manager.h"
#include "../../../include/algo/cpu/cpu_random_algo.h"

/* random_algo_manager_init
 * Initializes the random solver (if needed).
 */
int random_algo_manager_init(AntNetContext* ctx)
{
    if (!ctx)
    {
        return ERR_INVALID_CONTEXT;
    }
    /* No specific initialization required for random solver at this time. */
    return ERR_SUCCESS;
}

/* random_algo_manager_run
 * Executes the random solver from start_node to end_node.
 * Returns 0 (ERR_SUCCESS) or negative error code.
 */
int random_algo_manager_run(
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
    return random_search_path(
        ctx,
        start_node,
        end_node,
        out_nodes,
        max_size,
        out_length,
        out_latency
    );
}

/* random_algo_manager_cleanup
 * Cleans up any allocated resources for the random solver (if needed).
 */
int random_algo_manager_cleanup(AntNetContext* ctx)
{
    if (!ctx)
    {
        return ERR_INVALID_CONTEXT;
    }
    /* No specific cleanup required at this time. */
    return ERR_SUCCESS;
}
