//include/error_codes.h
#ifndef ERROR_CODES_H
#define ERROR_CODES_H

/*
 * error_codes.h
 * Central definitions of numeric constants for error/success codes.
 * Use these constants to return or compare error states in the backend.
 */

/* 0 = success */
#define ERR_SUCCESS                   0

/* General errors */
#define ERR_INVALID_ARGS             -1
#define ERR_NO_TOPOLOGY              -2
#define ERR_NO_PATH_FOUND            -3
#define ERR_ARRAY_TOO_SMALL          -4
#define ERR_UNIMPLEMENTED            -5
#define ERR_MEMORY_ALLOCATION        -6

/* context-specific codes */
#define ERR_INVALID_CONTEXT          -7
#define ERR_NO_FREE_SLOT             -8

#endif /* ERROR_CODES_H */
