/**
 * @file network_game_fermi.c
 * @brief Simulating games on a network via Fermi-rule updates.
 * @author Shakti N. Menon
 * @date 06-11-2026
 * @license Distributed under the MIT License (see LICENSE for details).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <sys/stat.h>
#include "cli_utils.h"
#include "network_loader.h"
#include "rng_xoshiro.h"

/*=============================================================================*/
int main (int argc, char *argv[])
{
	// process all input arguments and convert to appropriate formats
	double T=0.0, K=0.0;
	int tmax=0, rec_int=0;
	unsigned long long seed=0;
	if (process_args(argc, argv, &T, &K, &tmax, &seed, &rec_int) != 0) {
 	   return 1;
	}
	const char *network_file = argv[1]; // extract network file name from input

	xoshiro_init(seed); // set seed for RNG (xoshiro256++)

	printf( "Simulating network=%s, T=%.15g, K=%.15g, seed=%llu\n",
	        network_file, T, K, seed );

	// load network data [no of nodes, degree of each, links of each, and
	// maximum degree of the network]
	int N=0, max_deg=0, *deg=NULL, *links=NULL;
	if ( scan_network(network_file, &N, &deg, &links, &max_deg) != 0 ){
		return 1;
	}
	printf("Loaded network: N = %d, max(deg) = %d\n", N, max_deg);

	// extract filename stem and use it to set the output directory name
	char stem[96], out_dir[128];
	get_file_stem(network_file, stem, sizeof(stem));
	snprintf(out_dir, sizeof(out_dir), "data_%s", stem);

	// create output directory if it doesn't exist
	if (mkdir(out_dir, 0755) != 0 && errno != EEXIST) {
		fprintf(stderr, "Error: cannot create output folder\n%s\n",
			strerror(errno));
		free(deg); free(links);
		return 1;
	}

	// open file for writing output
	char fout[256];
	snprintf( fout, sizeof fout, "%s/nc_T_%.15g_K_%.15g_seed%llu.dat",
		out_dir, T, K, seed );
	FILE *f1 = fopen( fout, "w" );
	if ( f1 == NULL ){
		fprintf( stderr, "Error: cannot open %s for writing\n%s\n",
			fout, strerror(errno) );
		free(deg); free(links);
		return 1;
	}

	// open a file for recording the time-series if a (non-zero) value
	// of rec_int is provided in the input list 
	char fts_path[256];
	FILE *fts = NULL;
	int use_ts = (rec_int > 0);
	if (use_ts){
		snprintf(fts_path, sizeof fts_path,
				"%s/TS_T_%.15g_K_%.15g_seed%llu.dat", out_dir, T, K, seed);
		fts = fopen(fts_path, "w");
		if (fts == NULL){
			fprintf(stderr, "Error: cannot open %s for writing\n%s\n",
					fts_path, strerror(errno));
			free(deg); free(links);
			fclose(f1);
			return 1;
		}
	}

	// initialize arrays that store information for each agent
	int *act     = malloc((size_t)N * sizeof *act);
	int *act_nxt = malloc((size_t)N * sizeof *act_nxt);
	double *payoff  = malloc((size_t)N * sizeof *payoff);
	// exit if any of the arrays cannot be created
	if (!act || !act_nxt || !payoff) {
		fprintf(stderr, "Error: out of memory\n");
		free(deg); free(links);
		free(act); free(act_nxt); free(payoff);
		fclose(f1); if (fts) fclose(fts);
		return 1;
	}
	
	// initialize actions and payoffs of all agents
	for (int i = 0; i < N; i++ ){
		act[i] = (xoshiro_uniform() < 0.5) ? 1 : 0;
		payoff[i] = 0.0;
		act_nxt[i] = act[i];
	}

	/*-------------------------------------------------------------*/
	/*    Main loop; runs until final value of time is reached     */
	/*-------------------------------------------------------------*/
	int nc = 0; // number of cooperators in the population
	for (int i = 0; i < N; i++){ nc += act[i]; }

	// (optional) record t=0 with the initial state
	if (use_ts){
		if (fprintf(fts, "0 %d\n", nc) < 0){
			fprintf(stderr, "Error: cannot write to %s\n%s\n",
					fts_path, strerror(errno));
			fclose(f1); fclose(fts);
			free(deg); free(links);
			free(act); free(act_nxt); free(payoff);
			return 1;
		}
	}

	int tcurr  = 0;
	int absorbed = 0; // absorbing state reached?
	while ( tcurr < tmax ){

		// calculate the total payoffs received by each agent
		for (int i = 0; i < N; i++ ){
			int tot_coop_ngh = 0;
			for (int j = 0; j < deg[i]; j++ ){
				tot_coop_ngh += act[links[(size_t)i * (size_t)max_deg + (size_t)j]];
			}
			if ( act[i] == 1 )
			{payoff[i] = (double)tot_coop_ngh;}
			else
			{payoff[i] = T*(double)tot_coop_ngh;}
		}

		// synchronous update with Fermi rule
		for (int i = 0; i < N; i++ ){
			// skip isolated nodes
			if ( deg[i] == 0 ){
				act_nxt[i] = act[i];
				continue;
			}
			int ngh_indx = (int)floor((deg[i])*xoshiro_uniform());
			int rnd_ngh = links[(size_t)i * (size_t)max_deg + (size_t)ngh_indx];

			double prob;
			if (K==0.0){
				prob = (payoff[rnd_ngh] > payoff[i]) ? 1.0 : 0.0;
			} else{
				prob = 1.0/(1.0+exp(-(payoff[rnd_ngh]-payoff[i])/K));
			}

			if ( xoshiro_uniform() < prob )
			{act_nxt[i] = act[rnd_ngh];}
			else
			{act_nxt[i] = act[i];}
		}

		// update the actions of all agents
		for (int i = 0; i < N; i++ ){ act[i] = act_nxt[i]; }
		if (tcurr < tmax) tcurr++;
		
		nc = 0; // number of cooperators in the population
		for (int i = 0; i < N; i++ ){ nc += act[i]; }
		
		// record time series spaced by interval "rec_int"
		if ( use_ts && tcurr % rec_int == 0 ){
			if ( fprintf(fts, "%d %d\n", tcurr, nc) < 0 ){
					fprintf(stderr, "Error: cannot write to %s\n%s\n",
							fts_path, strerror(errno));
					fclose(f1);
					fclose(fts);
					free(deg); free(links);
					free(act); free(act_nxt); free(payoff);
					return 1;
			}
		}

		// exit if absorbing state is reached
		if ( nc == 0 || nc == N ){
			// record absorbing state in time series if required
			if ( use_ts && tcurr % rec_int != 0 ){
				if ( fprintf(fts, "%d %d\n", tcurr, nc) < 0 ){
					fprintf(stderr, "Error: cannot write to %s\n%s\n",
							fts_path, strerror(errno));
					fclose(f1); fclose(fts);
					free(deg); free(links); free(act);
					free(act_nxt); free(payoff);
					return 1;
				}
			}
			absorbed = 1;
			tcurr = tmax;
		}

	}
	/*-------------------------------------------------------------*/

	// if the simulation ended without reaching an absorbing state
	// and tmax was not a multiple of rec_int, record the final state
	if ( use_ts && !absorbed && tcurr > 0 && tcurr % rec_int != 0 ){
		if ( fprintf(fts, "%d %d\n", tcurr, nc) < 0 ){
			fprintf(stderr, "Error: cannot write to %s\n%s\n",
					fts_path, strerror(errno));
			fclose(f1); fclose(fts);
			free(deg); free(links);
			free(act); free(act_nxt); free(payoff);
			return 1;
		}
	}

	free(deg); free(links); free(act); free(act_nxt); free(payoff);

	if ( fprintf( f1, "%d\n", nc ) < 0 ){
		fprintf(stderr, "Error: cannot write to %s\n%s\n",
			fout, strerror(errno));
		fclose(f1); if (fts) fclose(fts);
		return 1;
	}

	if ( fclose(f1) != 0 ){
		fprintf(stderr, "Error: cannot close %s\n%s\n",
			fout, strerror(errno));
		if (fts) fclose(fts);
		return 1;
	}

	// close time series file if it was created
	if (fts != NULL){
		if (fclose(fts) != 0){
			fprintf(stderr, "Error: cannot close %s\n%s\n",
					fts_path, strerror(errno));
			return 1;
		}
	}

	puts("Done");
	return 0;
}
/*=============================================================================*/