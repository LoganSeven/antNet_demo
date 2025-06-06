/* Relative Path: src/c/algo/cpu/cpu_random_algo_path_reorder.c */
/*
 * Reorders intermediate nodes of a random-generated path by ascending node_id.
 * Leaves the boundary nodes intact, ensuring only a cosmetic or display-focused change.
 * Helps maintain a clear external representation without altering solver internals.
*/

#include "../../../../include/algo/cpu/cpu_random_algo_path_reorder.h"

/*
 * random_algo_reorder_path_for_display
 *
 * Reorders the path entries between the first and the last node by ascending node_id.
 * This approach is purely cosmetic, for external rendering, and does not affect
 * how the solver picks or stores its best path in the context.
 *
 * Path structure: [start_id, intermediate..., end_id].
 * This function sorts the sub-array [1..path_length-2], ignoring boundary nodes.
 */
void random_algo_reorder_path_for_display(int* path, int path_length)
{
    if (!path || path_length < 3) {
        return;
    }

    int* intermediates = path + 1;
    int count = path_length - 2;  /* number of intermediate nodes */

    /* Simple ascending sort by node_id */
    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            if (intermediates[j] < intermediates[i]) {
                int tmp = intermediates[i];
                intermediates[i] = intermediates[j];
                intermediates[j] = tmp;
            }
        }
    }
}
