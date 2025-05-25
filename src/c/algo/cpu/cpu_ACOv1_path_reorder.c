/* Relative Path: src/c/algo/cpu/cpu_ACOv1_path_reorder.c */
/*
 * Reorders the intermediate nodes of an ACO-generated path for external display.
 * Helps keep solver internals intact while returning a cleaner path sequence.
 * Purely cosmetic function, ignores actual optimization logic.
*/


#include "../../../../include/algo/cpu/cpu_ACOv1_path_reorder.h"

#include <stdlib.h>
#include <string.h>

/*
 * aco_v1_reorder_path_for_display
 *
 * Reorders the path entries between the first and the last node by ascending node_id.
 * This approach is purely for cosmetic or external representation, without changing
 * the internal solver logic or the stored best path in the context.
 * 
 * Path structure: [0, intermediate..., 1].
 * This function sorts the sub-array [1..path_length-2], ignoring boundary nodes.
 */
void aco_v1_reorder_path_for_display(int* path, int path_length)
{
    if (!path || path_length < 3) {
        return;
    }

    int *intermediates = path + 1;
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
