/* Relative Path: include/algo/cpu/cpu_ACOv1_path_reorder.h */
/*
 * cpu_ACOv1_path_reorder.h
 * Declares a helper function to reorder the intermediate nodes of a path
 * for display or frontend usage only.
 */

#ifndef CPU_ACOV1_PATH_REORDER_H
#define CPU_ACOV1_PATH_REORDER_H

/*
 * aco_v1_reorder_path_for_display
 * This function sorts the path entries between [1..(length-2)] in ascending order
 * of node IDs. The first (start=0) and last (end=1) nodes remain fixed.
 */
void aco_v1_reorder_path_for_display(int* path, int path_length);

#endif /* CPU_ACOV1_PATH_REORDER_H */
