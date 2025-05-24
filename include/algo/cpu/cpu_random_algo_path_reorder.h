/* Relative Path: include/algo/cpu/cpu_random_algo_path_reorder.h */
/*
 * cpu_random_algo_path_reorder.h
 * Declares a helper function to reorder the intermediate nodes of a path
 * for display or frontend usage only, in ascending order by node_id.
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
