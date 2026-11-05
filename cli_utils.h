#ifndef CLI_UTILS_H
#define CLI_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

static inline int process_args(char *argv[], double *T_out, double *K_out,
	int *tmax_out, int *rec_int_out, unsigned long long *seed_out)
{
	char *endptr; // pointer for strtod and strtol 

    errno = 0;
    double T = strtod(argv[2], &endptr);
    if (argv[2] == endptr || errno == ERANGE || *endptr != '\0') {
        fprintf(stderr, "Error: T ('%s') invalid/out-of-range\n", argv[2]);
        return 1;
    }

    errno = 0;
    double K = strtod(argv[3], &endptr);
    if (argv[3] == endptr || errno == ERANGE || *endptr != '\0') {
        fprintf(stderr, "Error: K ('%s') invalid/out-of-range\n", argv[3]);
        return 1;
    }

    errno = 0;
    long tmax_long = strtol(argv[4], &endptr, 10);
    if (argv[4] == endptr || errno == ERANGE || *endptr != '\0') {
        fprintf(stderr, "Error: tmax ('%s') invalid/out-of-range\n", argv[4]);
        return 1;
    }

    errno = 0;
    long rec_int_long = strtol(argv[5], &endptr, 10);
    if (argv[5] == endptr || errno == ERANGE || *endptr != '\0') {
        fprintf(stderr, "Error: rec_int ('%s') invalid/out-of-range\n", argv[5]);
        return 1;
    }

	errno = 0;
	unsigned long long seed = strtoull(argv[6], &endptr, 10);
	if (argv[6] == endptr || errno == ERANGE || *endptr != '\0') {
		fprintf(stderr, "Error: seed ('%s') invalid or out-of-range\n", argv[6]);
		return 1;
	}
	// reject negative input seeds
	if (argv[6][0] == '-' && argv[6][1] != '\0') {
		fprintf(stderr, "Error: seed must be non-negative\n");
		return 1;
	}

	// check if integers are less than the max allowed value
	if (tmax_long < 0 || tmax_long > INT_MAX) {
		fprintf(stderr, "Error: tmax out of range [0, %d]\n", INT_MAX);
		return 1;
	}
	if (rec_int_long <= 0 || rec_int_long > INT_MAX) {
		fprintf(stderr, "Error: rec_int out of range (1, %d]\n", INT_MAX);
		return 1;
	}
	// cast long back to int
    int tmax = (int)tmax_long;
    int rec_int = (int)rec_int_long;

	if (!(K >= 0.0)) {
		fprintf(stderr, "Error: K must be non-negative and finite\n");
		return 1;
	}
	if (!(T > -1e300 && T < 1e300)) {
		fprintf(stderr, "Error: T must be a finite real number\n");
		return 1;
	}

	*T_out      = T;
	*K_out      = K;
	*tmax_out   = tmax;
	*rec_int_out = rec_int;
	*seed_out = seed;
	return 0;	
}

// Helper to extract clean base name without extension
static inline void get_file_stem(const char *filepath, char *dest, size_t max_len) {
    const char *base = strrchr(filepath, '/');
    #ifdef _WIN32
    const char *backslash = strrchr(filepath, '\\');
    if (backslash && (!base || backslash > base)) base = backslash;
    #endif
    base = (base != NULL) ? base + 1 : filepath;

    snprintf(dest, max_len, "%s", base);
    char *dot = strrchr(dest, '.');
	if (dot != NULL && dot != dest) { *dot = '\0'; }

}

#endif