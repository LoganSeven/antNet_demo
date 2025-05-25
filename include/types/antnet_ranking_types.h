/* Relative Path: include/types/antnet_ranking_types.h */
/*
 * Declares the RankingEntry struct for storing algorithm name, SASA score, and best latency.
 * Used in the ranking system to compare solver performance at a glance.
*/


#ifndef ANTNET_RANKING_TYPES_H
#define ANTNET_RANKING_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * RankingEntry
 * Holds the name, current SASA score, and best latency (in ms) for one algorithm.
 */
typedef struct RankingEntry {
    char   name[8];
    double score;
    int    latency_ms;
} RankingEntry;

#ifdef __cplusplus
}
#endif

#endif /* ANTNET_RANKING_TYPES_H */
