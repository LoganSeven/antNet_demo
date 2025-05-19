/* Relative Path: include/types/antnet_sasa_types.h */
/*
 * antnet_sasa_types.h
 * Declares the structure holding SASA coefficients for each context.
 * This file is new and follows the existing style conventions.
 */

#ifndef ANTNET_SASA_TYPES_H
#define ANTNET_SASA_TYPES_H

/*
 * SasaCoeffs
 * Holds alpha, beta, gamma coefficients used by the SASA scoring logic.
 */
typedef struct SasaCoeffs {
    double alpha;
    double beta;
    double gamma;
} SasaCoeffs;

#endif /* ANTNET_SASA_TYPES_H */
