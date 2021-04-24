#ifndef EVOLVE_TURING_H
#define EVOLVE_TURING_H

#include "turing.h"

#define MAX_STEPS(TAPE) sizeof(TAPE)*sizeof(TAPE)*sizeof(TAPE)
#define TAPE_LEN 1000
#define SAMPLE_TAPE1 {BLANK,3,1,2,1,2,3,2,3,3,3,2,2,2,1,1,1,BLANK}
#define SAMPLE_TAPE2 {BLANK,3,2,1,3,2,1,3,2,1,3,2,1,3,2,1,1,1,1,BLANK}
#define SAMPLE_TAPE3 {BLANK,3,3,3,3,3,3,3,3,2,2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,BLANK}
#define NR_OF_SAMPLE_TAPES 3
#define SAMPLE_TAPE_SYMBOLS 4
#define TAPE_LOG_SIZE 65535

typedef struct {
	int population_size,
		states,
		symbols,
		best_cnt, kids_cnt, degeneration_cnt;
	char * output;
} tParams;


typedef struct {
	tTransTableItem * table;
	double fitness;
} tIndividual;

typedef struct {
	int symbol_count[SAMPLE_TAPE_SYMBOLS];
	int correct_order;
} tTapeMetrics; 

void calc_all_tapes_metrics(tTape * tapes, tTapeMetrics * metrics, int n);
double eval_sorting_fitness(tTransitions * t, tTape * tape, tTapeMetrics * orig_metrics);
double eval_sorting_fitness_n_tapes(tTransitions * t, tTape * orig_tapes, int n, char * tape_log);
int evolve_turing(tParams * params, tTape * orig_tapes, int nr_of_tapes);


#endif
