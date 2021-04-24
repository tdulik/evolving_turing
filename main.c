#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include <signal.h>
#include <ctype.h>
#include "common.h"
#include "turing.h"
#include "evolve_turing.h"
#include "pqueue.h"


#define TAPE_LEN 1000
typedef enum {
	start, was_1, was_2, was_3,  s2_1,  s3_1,  s3_2, was_2_swap, was_3_swap, search_blank, error, end
} tDemoBubbleStates;


tTransTableItem demoBubbleTable[]={
	// 0=start:	BLANK		1			2				3
	  {end, E, N}, {was_1, E, R}, {was_2, E, R}, {was_3, E, R} ,
	// 1=was_1:	BLANK		1			2				3
	  {end, E, N}, {was_1, E, R}, {was_2, E, R}, {was_3, E, R} ,
	// 2=was_2:	BLANK		1			2				3
	  {end, E, N}, { s2_1, 2, L}, {was_2, E, R}, {was_3, E, R} ,
	// 3=was_3:	BLANK		1			2				3
	  {end, E, N}, { s3_1, 3, L}, { s3_2, 3, L}, {was_3, E, R} ,
	// 4=s2_1:	BLANK		1			2				3
	  {error, E, N}, {error, E, N}, {was_2_swap, 1, RR} , {error, E, N},
	// 5=s3_1:	BLANK		1			2				3
	  {error, E, N}, {error, E, N}, {error, E, N}, {was_3_swap, 1, RR} ,
	// 6=s3_2:	BLANK		1			2				3
	  {error, E, N}, {error, E, N}, {error, E, N}, {was_3_swap, 2, RR} ,
	// 7=was_2_swap:BLANK		1			2				3
	  {search_blank, E, L}, { s2_1, 2, L}, {was_2_swap, E, R},  {was_3_swap, E, R} ,
	// 8=was_3_swap:BLANK		1			2				3
	  {search_blank, E, L}, { s3_1, 3, L}, { s3_2, 3, L},  {was_3_swap, E, R} ,
	// 9=search_blank:	BLANK		1			2				3
	  {start, E, R}, {search_blank, E, L}, {search_blank, E, L}, {search_blank, E, L} 
	// 10=error (final state)
	// 11=end (final state)
};

#define DEMOBUBBLE_SYMBOLS 4
tTransitions demoBubble = {	
		sizeof(demoBubbleTable)/sizeof(tTransTableItem)/DEMOBUBBLE_SYMBOLS,
		//=10,									// nr. of states
	DEMOBUBBLE_SYMBOLS,					// nr. of symbols
	demoBubbleTable 					// transition table
};

tTape Sample_tapes[] = {
		{SAMPLE_TAPE1, sizeof((char[])SAMPLE_TAPE1)},
		{SAMPLE_TAPE2, sizeof((char[])SAMPLE_TAPE2)},
		{SAMPLE_TAPE3, sizeof((char[])SAMPLE_TAPE3)},
};

void help_exit(char * progname) {
	printf("%s [-b NR_OF_BESTS] [-k NR_OF_KIDS] [-o OUTPUT] [-p POPULATION_SIZE] "
			"[-s STATES] [-y SYMBOLS]\nwhere:\n"
			"-b BEST_CNT\n	sets the number of best individuals, who are evolved. Default is 5000\n"
			"-d DEGENARTION_CNT\n	if this number generations has no success, then the evolution is restarted. Default is 500\n"
			"-k KIDS_CNT\n	sets the number of kids of the best individual. Default is 10\n"
			"-p POPULATION_SIZE\n	sets the population size. Default value is 10000\n"
			"-s STATES\n	sets the number of Turing machine states. Default value is 12\n"
			"-y SYMBOLS\n	sets the number of Turing machine symbols. Default value is 4\n"
			"-o OUTPUT\n	output directory. Default is \"output\"\n", progname);
	exit(EXIT_SUCCESS);
}

/**
 * @TODO Add options for:
 *  - sample tapes. They would be part of tParams type. Result=lots of simplification
 *  - importing good individuals for starting evolution where we ended
 */
void get_options(int argc, char ** argv, tParams * params) {
	int i;
	long val;
	char * arg, * endptr;
	enum {best, degeneration, kids, output, popul_size, states, symbols} arg_type=popul_size;
	for (i=1; i<argc; i++) {
		arg=argv[i];
		if (arg[0]=='-')
			switch (arg[1]) {
				case 'b': arg_type=best; break;
				case 'd': arg_type=degeneration; break;
				case 'k': arg_type=kids; break;
				case 'o': arg_type=output; break;
				case 'p': arg_type=popul_size; break;
				case 's': arg_type=states; break;
				case 'y': arg_type=symbols; break;
			default:
				help_exit(argv[0]);
			}
		else {
			if (arg_type==output)
				params->output=arg;
			else {
				val=strtol(arg, &endptr, 10);
				if (endptr==arg) help_exit(argv[0]);
				switch (arg_type) {
					case popul_size: params->population_size=val; break;
					case symbols: params->symbols=val; break;
					case states: params->states=val; break;
					case kids: params->kids_cnt=val; break;
					case best: params->best_cnt=val; break;
					case degeneration: params->degeneration_cnt=val; break;					default:;
				}	// switch (arg_type)
			}
		} // else
	} // for
	printf("Parameters: population size=%d, states=%d, symbols=%d, best_cnt=%d, kids_cnt=%d, degeneration_cnt=%d\n",
			params->population_size, params->states, params->symbols, params->best_cnt, params->kids_cnt, params->degeneration_cnt);
}

tParams params={10000, 12, 4, 5000, 10, 1000, "output"};

volatile int log_level=LOG_NONE_0;
void sighandler(int sig)
{
	int val, old_log_level=log_level;
	log_level=LOG_NONE_0;		// disable logging while waiting for user input...
	char line[255];
	signal(SIGINT, &sighandler); // reestablish this as signal handler
	printf("\nCurrent parameters: log_level=%d, population size=%d, states=%d, symbols=%d,\n"
			"best_cnt=%d, kids_cnt=%d, degeneration_cnt=%d\n",
			old_log_level, params.population_size, params.states, params.symbols,
			params.best_cnt, params.kids_cnt, params.degeneration_cnt);
	printf("Enter <0..3> as log_level | [b BEST_CNT] | [d DEGENERATION_CNT] [k KIDS_CNT], 'c' for continue, anything else for exit:\n");
	if (fgets(line, 255, stdin)!=NULL) {
		if (isdigit(line[0])) {
			val=atoi(line);
			if (val<0 || val>3)
				fprintf(stderr, "Log level must be in range <0,3>!\n");
			else log_level=val;
		} else {
			val=atoi(line+1);
			switch (tolower(line[0])) {
				case 'b': params.best_cnt=val; break;
				case 'd': params.degeneration_cnt=val; break;
				case 'k':
					if (val <= params.best_cnt)
						params.kids_cnt = val;
					else fprintf(stderr, "KIDS_CNT must be < best_cnt=%d\n", params.best_cnt);
					break;
				case 'c': break;
				default: exit(EXIT_SUCCESS);
			}
			log_level=old_log_level;
		}
	}
	printf("New parameters: log_level=%d, population size=%d, states=%d, symbols=%d,\n"
			"best_cnt=%d, kids_cnt=%d, degeneration_cnt=%d\n",
			log_level, params.population_size, params.states, params.symbols,
			params.best_cnt, params.kids_cnt, params.degeneration_cnt);
}

int main(int argc, char **argv) {
	char log[20000];
	int cpus=omp_get_num_procs();
	int n=sizeof(Sample_tapes)/sizeof(tTape);
	tTapeMetrics metrics[n];

	signal(SIGINT, &sighandler);

	get_options(argc, argv, &params);
	calc_all_tapes_metrics(Sample_tapes, metrics, n);
	printf("Using CPUs=%d\n", cpus);
	//log_level=LOG_ALL_2;
	eval_sorting_fitness_n_tapes(&demoBubble, Sample_tapes, n, log);
	#pragma omp parallel num_threads(cpus)
		evolve_turing(&params, Sample_tapes, n);

	return 0;

}

