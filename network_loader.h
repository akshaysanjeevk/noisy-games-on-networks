/**
 * @file network_loader.h
 * @brief Header-only utility to parse, validate, and load network files.
 * @author Shakti N. Menon
 * @date 06-11-2026
 * @license Distributed under the MIT License (see LICENSE for details).
 */

#ifndef NETWORK_LOADER_H
#define NETWORK_LOADER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/**
 * @brief Scan an input network file, parse node degrees, and load neighbor links.
 * 
 * Performs a two-pass scan of the network file to determine the number of nodes, 
 * individual node degrees, maximum degree, and the flattened 2D connectivity array.
 *
 * @param filename   Path to the network data file.
 * @param N_out      Pointer to store the total number of nodes (N).
 * @param deg_out    Pointer to store the dynamically allocated array of node degrees.
 * @param links_out  Pointer to store the dynamically allocated flattened 2D links array.
 * @param maxdeg_out Pointer to store the maximum degree found in the network.
 * @return int       Returns 0 on success, or -1 on failure (with details logged to stderr).
 */
static inline int scan_network(const char *filename, int *N_out, int **deg_out,
						int **links_out, int *maxdeg_out)
{
	FILE *fp = fopen( filename, "r" );
	if ( fp == NULL ){
		fprintf( stderr, "Error: cannot open %s\n%s\n", filename, strerror(errno) );
		return -1;
	}

	int cap = 1024; // initial capacity for the degree array
	int *deg = (int *)malloc((size_t)cap * sizeof *deg); // allocate heap memory
	if (deg == NULL){
		fprintf(stderr, "Error: out of memory\n");
		fclose(fp);
		return -1;
	}

	int N = 0; // number of nodes in the network file
	int c = 0; // current character read from the file
	int row_deg = 0; // degree of current row
	int prev_ws = 1; // boolean : is the previous character whitespace?

	// first pass: cycle through all characters in file until EOF is reached
	while( (c = fgetc(fp)) != EOF ){
		// end of row reached -> record degree
		if ( c == '\n' ){
			// growth check (if array exceeded preset cap)
			if ( N >= cap ){
				// double the capacity and reallocate heap memory
				cap *= 2;
				int *tmp = (int *)realloc(deg, (size_t)cap * sizeof *deg);
				// memory allocation failure => exit
				if (tmp == NULL){ free(deg); fclose(fp); return -1; }
				deg = tmp;
			}
			deg[N++] = row_deg; // store degree for this node

			row_deg = 0; // reset column count
			prev_ws = 1;
		} else {
			// check if current character is whitespace
			int is_ws = ( c == ' ' || c == '\t' || c == '\r' );

			// increment column count if prev. character was whitespace but
			// the current character isn't
			if ( !is_ws && prev_ws ){ row_deg++; }
			prev_ws = is_ws;
		}
	}

	// if the final line doesn't have a trailing newline, row_deg will be > 0
	// and for this case we should increment N and record the degree
	if ( row_deg > 0 ){
		if ( N >= cap ){
			cap *= 2;
			int *tmp = (int *)realloc(deg, (size_t)cap * sizeof *deg);
			if (tmp == NULL){ free(deg); fclose(fp); return -1; }
			deg = tmp;
		}
		deg[N++] = row_deg;
	}

	// exit if the file contains no rows
	if ( N == 0 ){
		fprintf(stderr, "Error: no network data\n");
		free(deg); fclose(fp);
		return -1;
	}

	// compute maximum degree of the network to determine the column
	// width of the flattened 2D "links" array
	int maxdeg = 0;
	for (int r = 0; r < N; r++){
		if ( deg[r] > maxdeg ){ maxdeg = deg[r]; }
	}
	if ( maxdeg == 0 ){
		fprintf(stderr, "Error: empty graph\n");
		free(deg); fclose(fp);
		return -1;
	}

	// Allocate heap memory for the 2D "links" array (N rows, maxdeg columns)
    int *links = (int *)malloc((size_t)N * (size_t)maxdeg * sizeof *links);
    if (links == NULL) {
        fprintf(stderr, "Error: out of memory for links\n");
        free(deg); fclose(fp);
        return -1;
    }

	// Rewind file to the beginning for the second pass:
	// read the network neighbour indices and assign to links array
	if (fseek(fp, 0, SEEK_SET) != 0) {
		fprintf(stderr, "Error: cannot rewind file %s\n%s\n",
			filename, strerror(errno));
		free(links); free(deg); fclose(fp);
		return -1;
	}

	// second pass: parse neighbour indices for each node (row)
	for (int i = 0; i < N; i++) {
        for (int ngh = 0; ngh < deg[i]; ngh++) {
            // verify that fscanf successfully read an integer from the file
            if (fscanf(fp, "%d",
				&links[(size_t)i * (size_t)maxdeg +(size_t)ngh]) != 1) {
				fprintf(stderr,
					"Error: expected an integer in (row %d, col %d) of %s\n",
					i, ngh, filename);
                free(links);
                free(deg);
                fclose(fp);
                return -1;
            }
			// validate that the neighbour indices lie within the valid range
			if (links[(size_t)i * (size_t)maxdeg + (size_t)ngh] < 0
				|| links[(size_t)i * (size_t)maxdeg + (size_t)ngh] >= N) {
				fprintf(stderr, "Error: idx %d not in [0,%d) in %s (row %d)\n",
					links[(size_t)i * (size_t)maxdeg + (size_t)ngh], N, filename, i);
                free(links);
                free(deg);
                fclose(fp);
                return -1;
			}
        }
    }

	// parsing and validation complete - close file stream
    fclose(fp);

	// assign output pointers
	*N_out     = N;
	*deg_out   = deg;
	*links_out = links;
	*maxdeg_out = maxdeg;

	return 0;	
}

#endif // NETWORK_LOADER_H