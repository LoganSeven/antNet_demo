/* Relative Path: src/c/managers/cpu_brute_force_algo_manager.c */
/*
 * cpu_brute_force_algo_manager.c
 * Implementation of the manager for the brute-force search algorithm in AntNet.
 */

#include "../../../include/core/backend.h"
#include "../../../include/managers/cpu_brute_force_algo_manager.h"
#include "../../../include/algo/cpu/cpu_brute_force.h"
#include <string.h>

/* brute_force_algo_manager_init
 * Initializes the brute-force solver (if needed).
 */
int brute_force_algo_manager_init(AntNetContext* ctx)
{
    if (!ctx)
    {
        return ERR_INVALID_CONTEXT;
    }
    /* No specific initialization required for brute force at this time. */
    return ERR_SUCCESS;
}

/* brute_force_algo_manager_run
 * Executes a brute-force solver step from start_node to end_node.
 * Returns 0 (ERR_SUCCESS) or negative error code.
 */
int brute_force_algo_manager_run(
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
    return brute_force_search_step(
        ctx,
        start_node,
        end_node,
        out_nodes,
        max_size,
        out_length,
        out_latency
    );
}

/* brute_force_algo_manager_cleanup
 * Cleans up any allocated resources for the brute-force solver (if needed).
 */
int brute_force_algo_manager_cleanup(AntNetContext* ctx)
{
    if (!ctx)
    {
        return ERR_INVALID_CONTEXT;
    }
    /* No specific cleanup required at this time. */
    return ERR_SUCCESS;
}
