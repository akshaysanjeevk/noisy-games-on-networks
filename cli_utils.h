/**
 * @file cli_utils.h
 * @brief Command-line argument processing utilities.
 * @author Shakti N. Menon
 * @date 06-11-2026
 * @license Distributed under the MIT License (see LICENSE for details).
 */

#ifndef CLI_UTILS_H
#define CLI_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <limits.h>

/**
 * @brief Parse and validate all command-line arguments to main().
 * 
 * Expected command-line arguments:
 *   argv[0]: compiled script
 *   argv[1]: path to file with network adjacency list
 *   argv[2]: T (temptation to defect) [double]
 *   argv[3]: K (Fermi-rule noise) [double]
 *   argv[4]: tmax (maximum time steps in simulation) [int]
 *   argv[5]: seed for RNG [unsigned long long]
 *   argv[6]: rec_int (interval between recordings) [int] (optional)
 *
 * @param argc        Argument count from main().
 * @param argv        Argument vector from main().
 * @param T_out       Pointer to store parsed T.
 * @param K_out       Pointer to store parsed K.
 * @param tmax_out    Pointer to store parsed tmax.
 * @param seed_out    Pointer to store parsed seed.
 * @param rec_int_out Pointer to store parsed rec_int (0=>don't save time series).
 * @return int        Returns 0 on success, or 1 on validation error.
 */
static inline int process_args(int argc, char *argv[], double *T_out,
	double *K_out, int *tmax_out, unsigned long long *seed_out, int *rec_int_out)
{
	// Ensure all required arguments were passed to prevent out-of-bounds access
	if ( argc != 6 && argc != 7 ){
		fprintf( stderr,
			"Usage: %s <network_file> <T> <K> <tmax> <seed> [<rec_int>]\n"
			"  network_file : index-based network adjacency list\n"
			"  T            : temptation to defect\n"
			"  K            : Fermi-rule noise\n"
			"  tmax         : total number of iterations\n"
			"  seed         : seed for RNG (xoshiro256++)\n"
			"  rec_int      : interval between successive recordings\n"
			"                 (optional; omit to skip time-series output)\n",
			argv[0] );
		return 1;
	}

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
	unsigned long long seed = strtoull(argv[5], &endptr, 10);
	if (argv[5] == endptr || errno == ERANGE || *endptr != '\0') {
		fprintf(stderr, "Error: seed ('%s') invalid or out-of-range\n", argv[5]);
		return 1;
	}
	// reject negative input seeds
	if (argv[5][0] == '-' && argv[5][1] != '\0') {
		fprintf(stderr, "Error: seed must be non-negative\n");
		return 1;
	}

    int rec_int = 0; // 0 means "don't record time series"
	if (argc == 7){
		errno = 0;
		long rec_int_long = strtol(argv[6], &endptr, 10);
		if (argv[6] == endptr || errno == ERANGE || *endptr != '\0'){
			fprintf(stderr, "Error: rec_int ('%s') invalid/out-of-range\n", argv[6]);
			return 1;
		}
		if (rec_int_long <= 0 || rec_int_long > INT_MAX){
			fprintf(stderr, "Error: rec_int out of range (1, %d]\n", INT_MAX);
			return 1;
		}
		rec_int = (int)rec_int_long;
	}

	// check if integers are less than the max allowed value
	if (tmax_long < 0 || tmax_long > INT_MAX) {
		fprintf(stderr, "Error: tmax out of range [0, %d]\n", INT_MAX);
		return 1;
	}
	// cast long back to int
    int tmax = (int)tmax_long;

	if (!(K >= 0.0)) {
		fprintf(stderr, "Error: K must be non-negative and finite\n");
		return 1;
	}
	if (!isfinite(T)) {
		fprintf(stderr, "Error: T must be a finite real number\n");
		return 1;
	}

	*T_out       = T;
	*K_out       = K;
	*tmax_out    = tmax;
	*seed_out    = seed;
	*rec_int_out = rec_int;
	return 0;	
}

/**
 * @brief Helper for extracting the clean base of a specified filename without
 * path prefixes or file extensions.
 * 
 * @param filepath Path to the file (e.g., "networks/links.dat").
 * @param dest     Destination buffer for storing the extracted stem.
 * @param max_len  Maximum capacity of the destination buffer.
 * @return void
 */
static inline void get_file_stem(const char *filepath, char *dest, size_t max_len) {
	// align pointer "base" to the last "/" in the filepath
    const char *base = strrchr(filepath, '/');

	// shift pointer by 1 such that it starts after the "/"
	// if no "/" is found (base==NULL) the pointer should start at the beginning of filepath
    base = (base != NULL) ? base + 1 : filepath;

	// copy the filename into "dest"
    snprintf(dest, max_len, "%s", base);

	// find the "." in the saved filename
    char *dot = strrchr(dest, '.');

	// strip file extension by replacing "." with a null terminator (\0).
	if (dot != NULL && dot != dest) { *dot = '\0'; }

	return;
}

#endif