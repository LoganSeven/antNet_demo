/* Relative Path: include/algo/cpu/cpu_random_algo_path_reorder.h */
/*
 * Defines a utility for reordering intermediate nodes in a randomly generated path.
 * Preserves the start/end nodes and sorts only the middle portion by ascending node_id.
 * Strictly cosmetic for clearer visualization.
*/


#ifndef CPU_RANDOM_ALGO_PATH_REORDER_H
#define CPU_RANDOM_ALGO_PATH_REORDER_H

/*
 * random_algo_reorder_path_for_display
 * Sorts the sub-array [1..(path_length-2)] by ascending node ID, leaving
 * path[0] (start_id) and path[path_length-1] (end_id) untouched.
 */
void random_algo_reorder_path_for_display(int* path, int path_length);

#endif /* CPU_RANDOM_ALGO_PATH_REORDER_H */
