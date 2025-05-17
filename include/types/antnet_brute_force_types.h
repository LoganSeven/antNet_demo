/* Relative Path: include/types/antnet_brute_force_types.h */

#ifndef ANTNET_BRUTE_FORCE_TYPES_H
#define ANTNET_BRUTE_FORCE_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * BruteForceState: internal iteration state for the brute force solver
 * stored in the AntNetContext.
 */
typedef struct {
    /*
     * candidate_nodes: list of node IDs that are neither start_id nor end_id.
     * candidate_count: how many such nodes.
     */
    int candidate_nodes[1024];
    int candidate_count;

    /*
     * current_L: current number of intermediate hops to examine (in [min_hops..max_hops])
     */
    int current_L;

    /*
     * permutation[]: an index-based permutation over [0..candidate_count-1].
     * The solver uses only the first current_L items to form a path.
     */
    int permutation[1024];

    /*
     * combination[]: an index-based combination over [0..candidate_count-1].
     * The solver uses this to select subsets of nodes before permuting them.
     */
    int combination[1024];

    /*
     * at_first_permutation: indicates if the solver has not called next_permutation() yet
     * for the current combination. If 1, the solver must initialize permutation in ascending order.
     */
    int at_first_permutation;

    /*
     * at_first_combination: indicates if the solver has not yet started combinations
     * for the current_L. If 1, the solver must initialize combination in ascending order.
     */
    int at_first_combination;

    /*
     * done: 1 if the solver enumerated all possible path lengths and permutations,
     * otherwise 0 if still in progress.
     */
    int done;

} BruteForceState;

#ifdef __cplusplus
}
#endif

#endif /* ANTNET_BRUTE_FORCE_TYPES_H */
