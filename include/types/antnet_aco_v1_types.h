/* Relative Path: include/types/antnet_aco_v1_types.h */

/* include/antnet_aco_v1_types.h */
#ifndef ANTNET_ACO_V1_TYPES_H
#define ANTNET_ACO_V1_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * AcoV1State: holds internal data for the ACO solver,
 * including pheromone matrix, adjacency, and solver parameters.
 * This structure is stored in AntNetContext as aco_v1.
 * This must never be removed, only adapted if design changes.
 */
typedef struct AcoV1State
{
    /* adjacency matrix: 1 if an edge (i->j) exists, else 0 */
    int* adjacency;
    int  adjacency_size; /* number of nodes, used to index adjacency[i*size + j] */

    /* pheromone matrix: pheromones[i*size + j] holds the pheromone for i->j */
    float* pheromones;
    int    pheromone_size; /* same as adjacency_size, for convenience */

    /* ACO hyper-parameters */
    float alpha;        /* importance of pheromone */
    float beta;         /* importance of heuristic (1/delay_ms) */
    float evaporation;  /* evaporation rate [0..1] */
    float Q;            /* deposit factor for pheromone update */
    int   num_ants;     /* number of ants each iteration */

    /* 0=not init, 1=init. Used to avoid re-initializing */
    int   is_initialized;
} AcoV1State;

#ifdef __cplusplus
}
#endif

#endif /* ANTNET_ACO_V1_TYPES_H */
