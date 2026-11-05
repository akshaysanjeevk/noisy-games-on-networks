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
	if ( argc != 7 ){
		fprintf( stderr,
			"Usage: %s <network_file> <T> <K> <tmax> <rec_int> <seed>\n"
			"  network_file : index-based network adjacency list\n"
			"  T            : temptation to defect\n"
			"  K            : Fermi-rule noise (typically in [1e-3, 1e0])\n"
			"  tmax         : total number of iterations (around 1e3-1e4)\n"
			"  rec_int      : interval between successive recordings (around 1e2)\n"
			"  seed         : seed for RNG (xoshiro256++)\n",
			argv[0] );
		return 1;
	}

	// process all input arguments and convert to appropriate formats
	double T=0.0, K=0.0;
	int tmax=0, rec_int=0;
	unsigned long long seed=0;
	if (process_args(argv, &T, &K, &tmax, &rec_int, &seed) != 0) {
 	   return 1;
	}
	// extract network file name from input
	const char *network_file = argv[1];

	xoshiro_init(seed);

	printf( "Simulating network=%s, T=%.15g, K=%.15g, seed=%llu\n",
	        network_file, T, K, seed );

	// load network data [no of nodes, degree of each, links of each, and
	// maximum degree of the network]
	int N=0, max_deg=0, *deg=NULL, *links=NULL;
	if ( scan_network(network_file, &N, &deg, &links, &max_deg) != 0 ){
		return 1;
	}
	printf("Loaded network: N = %d, max(deg) = %d\n", N, max_deg);

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

	// initialize arrays that store information for each agent
	int *action     = malloc((size_t)N * sizeof *action);
	int *action_new = malloc((size_t)N * sizeof *action_new);
	double *payoff  = malloc((size_t)N * sizeof *payoff);
	// exit if any of the arrays cannot be created
	if (!action || !action_new || !payoff) {
		fprintf(stderr, "Error: out of memory\n");
		free(deg);
		free(links);
		free(action);
		free(action_new);
		free(payoff);
		fclose(f1);
		return 1;
	}
	
	for (int i = 0; i < N; i++ ){
		action[i] = (int)floor(0.5 + xoshiro_uniform());
		payoff[i] = 0.0;
		action_new[i] = action[i];
	}

	/*-------------------------------------------------------------*/
	/*    Main loop; runs until final value of time is reached     */
	/*-------------------------------------------------------------*/
	int tcurr  = 0;
	int nc = 0; // number of cooperators in the population
	while ( tcurr < tmax ){

		// calculate the total payoffs received by each agent
		for (int i = 0; i < N; i++ ){
			payoff[i] = 0.0;
			int tot_coop_ngh = 0;
			for (int j = 0; j < deg[i]; j++ ){
				tot_coop_ngh += action[links[(size_t)i * (size_t)max_deg + (size_t)j]];
			}
			if ( action[i] == 1 )
			{payoff[i] = (double)tot_coop_ngh;}
			else
			{payoff[i] = T*(double)tot_coop_ngh;}
		}

		// synchronous update with Fermi rule
		for (int i = 0; i < N; i++ ){
			// skip isolated nodes
			if ( deg[i] == 0 ){
				action_new[i] = action[i];
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
			{action_new[i] = action[rnd_ngh];}
			else
			{action_new[i] = action[i];}
		}

		// calculate fraction of cooperators after interval "rec_int"
		if ( tcurr % rec_int == 0 ){
			nc = 0; // number of cooperators in the population
			for (int i = 0; i < N; i++ ){ nc += action_new[i]; }
			if ( nc == 0 || nc == N ){ tcurr = tmax; }
		}

		// update the actions of all agents
		for (int i = 0; i < N; i++ ){ action[i] = action_new[i]; }

		if (tcurr < tmax) tcurr++;
	}
	/*-------------------------------------------------------------*/

	free(deg);
	free(links);
	free(action);
	free(action_new);
	free(payoff);

	if ( fprintf( f1, "%d\n", nc ) < 0 ){
		fprintf(stderr, "Error: cannot write to %s\n%s\n",
			fout, strerror(errno));
		fclose(f1);
		return 1;
	}

	if ( fclose(f1) != 0 ){
		fprintf(stderr, "Error: cannot close %s\n%s\n",
			fout, strerror(errno));
		return 1;
	}

	puts("Done");
	return 0;
}
/*=================================END OF FILE=================================*/