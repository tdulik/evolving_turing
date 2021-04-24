#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <omp.h>
#include "turing.h"
#include "evolve_turing.h"
#include "pqueue.h"
#include "common.h"

tTransTableItem * Pregen_tuples;
int Pregen_tuples_cnt;
#pragma omp threadprivate(Pregen_tuples, Pregen_tuples_cnt)

inline int get_max_steps(int input_len) {
	return input_len*input_len*input_len;
}

void calc_tape_metrics(tTape * tape, tTapeMetrics *metrics) {
	signed char sym=0, prevSym;
	int i, first=1;
	int symbols=sizeof(metrics->symbol_count)/sizeof(*(metrics->symbol_count));
	// init of the symbol frequency counter
	for (i=0; i<symbols; i++) metrics->symbol_count[i]=0;
	metrics->correct_order=0;
	// calculate the symbol frequency and number of correctly ordered pairs
	for (i=1; i<tape->input_len; i++) {
		prevSym=sym;
		sym=tape->content[i];
		if (first) first=0;
		else 
			if (sym >= prevSym) metrics->correct_order++;
		metrics->symbol_count[sym]++;
	}
	tape->metrics=metrics;
}

void calc_all_tapes_metrics(tTape * tapes, tTapeMetrics * metrics, int n) {
	tTape * tape=tapes;
	tTapeMetrics * metric=metrics;
	int i;
	for (i=0; i<n; i++, tape++, metric++) {
		calc_tape_metrics(tape, metric);
	}
}

inline void init_tape(tTape * orig_tape, tTape * work_tape) {
	int len=orig_tape->input_len;
	memcpy(work_tape->content, orig_tape->content, len);
	memset(work_tape->content+len, BLANK, TAPE_LEN-len);
	work_tape->input_len=orig_tape->input_len;
}


void init_evolution(int states, int symbols) {
	int symbol, state, shift, i=0;
	/**
	 *  (states+1), because final state with nr. "states" is in the table content,
	 *  			but is not used as index!
	 *  (symbols+1), because symbol=-1 (Empty=no write) is in the table content
	 *  			 but is not used as index!
	 *  */
	Pregen_tuples_cnt=(states+1)*(symbols+1)*SHIFTS;
	Pregen_tuples=malloc(Pregen_tuples_cnt * sizeof(tTransTableItem));

	for (shift=0; shift<SHIFTS; shift++)
		for (symbol=-1; symbol<symbols; symbol++)
			for (state=0; state<=states; state++) {
				Pregen_tuples[i].state=state;
				Pregen_tuples[i].symbol=symbol;
				Pregen_tuples[i++].shift=shift;
			}
}

double eval_sorting_fitness(tTransitions * t, tTape * tape, tTapeMetrics * orig_metrics) {
	/**
	 * first, we measure the number of correctly ordered pairs
	 * and compare the count of the distinct symbols with the original.
	 * Then we calculate the fitness from these 2 numbers + nr. of steps and new_symbols written
	 */
	tStatus status = { 0, 0, 0, 0, 0};
	tTapeMetrics new_metrics;
	int i, correct_count, orig_unordered_cnt, delta_ordered_cnt, max_steps;
	double fit_correct, fit_time, fit_space; 
	// init of the symbol frequency counter

	max_steps=get_max_steps(tape->input_len);
	turing(tape, t, max_steps, &status);

	/* for completely wrong results, there is no need to calculate fitness...
	if (status.error<0) return -1;
	 */
	calc_tape_metrics(tape, &new_metrics);
	for (i=0, correct_count=0; i<t->symbols; i++) 
		if (orig_metrics->symbol_count[i]==new_metrics.symbol_count[i]) correct_count++;

	orig_unordered_cnt=tape->input_len-orig_metrics->correct_order-2-1; // -2=two BLANKs, -1 = usual "magic 1" 
	delta_ordered_cnt=new_metrics.correct_order - orig_metrics->correct_order;
	if (orig_unordered_cnt<1) {
		orig_unordered_cnt=1; //can't divide by 0
		if (delta_ordered_cnt>=0) delta_ordered_cnt=1;
	}
	/**
	 * Correctness = 0.5*Correct_symbol_count + 0.5*Delta_of_correctly_ordered_pairs  
	 */ 
	fit_correct=((double)correct_count/t->symbols + (double)delta_ordered_cnt/orig_unordered_cnt)/2;
	fit_time=1-(double)(status.steps + status.writes)/(2*max_steps);
	fit_space=1-(double)(2+status.head_max-tape->input_len)/(2+TAPE_LEN-tape->input_len);
	if (log_level>=LOG_DEBUG_3) {
		printf("Fitness: correctness=%.2lf, time complexity=%.2lf, space complexity=%.2lf\n",
				fit_correct, fit_time, fit_space);
	}
	return 0.5*fit_correct + 0.25*fit_time + 0.25*fit_space;
			 
}


double eval_sorting_fitness_n_tapes(tTransitions * t, tTape * orig_tapes, int n, char * tape_log) {
	tTape  work_tapes[n],
		 * work_tape=work_tapes, * orig_tape=orig_tapes;
	char * tape_log_start=tape_log;
	double fitness, result=0;
	int i, j;
	for (i=0; i<n; i++, orig_tape++, work_tape++) {
		init_tape(orig_tape, work_tape);
		fitness=eval_sorting_fitness(t, work_tape, orig_tape->metrics);
		if (fitness<0) return -1;
		else result+=fitness;

		for (j=0; j<work_tape->input_len; j++)
			if (tape_log < tape_log_start + TAPE_LOG_SIZE - 10)
				tape_log+=sprintf(tape_log, "%d,", work_tape->content[j]);

		tape_log+=sprintf(tape_log, "\n");

		if (log_level>=LOG_ALL_2)
			puts(tape_log_start);

	}
	if (log_level>=LOG_ALL_2) printf("Fitness sum=%.2lf\n", result);	
	return result;
}


unsigned long seed;
void generate_population(tTransTableItem * population, tIndividual * population_fitness,
					 tParams * params) {
	int i, st, sy;
	tTransTableItem * individual, * transition;
	tTransitions trans={params->states, params->symbols};

	#pragma omp atomic
	seed+=10000;

	srand (seed+time(NULL));

	individual=transition=population;
	for (i=0; i<params->population_size; i++) { // for all the individuals
		if (log_level>=LOG_DEBUG_3)
			printf("Generating individual %d\n", i);
		for (st=0; st<params->states; st++)			// for all their states
			for (sy=0; sy<params->symbols; sy++)	// for all their symbols
				*transition++=Pregen_tuples[rand()%Pregen_tuples_cnt];
		trans.table=population_fitness[i].table=individual;	
		individual=transition;
	}
}


inline int nr_of_best(int generation, int population_size) {
	int divisor;
	if (population_size>100) divisor=4;//+generation/2;
	else divisor=2;//+generation/4;
	if (divisor*2 > population_size) return 2;
	else return population_size/divisor;
}

inline int nr_of_kids(int generation, int i, int population_size) {
	return 10;
	if (population_size>10000) return population_size/100;
	else if (population_size>1000) return population_size/10;
	else if (population_size>100) return population_size/5;
	else return population_size/3;
}

inline void mutate(tIndividual * parent, tIndividual * kid, int states, int symbols) {
	int table_size=states*symbols, trans_nr, mutations=rand()%table_size, i;
	//first of all: copy the parent table into the kid's table
	for (i=0; i<table_size; i++) kid->table[i]=parent->table[i];
	//then, make the mutation(s)
	for (i=0; i<mutations; i++) {
		trans_nr=rand()%table_size;
		kid->table[trans_nr]=Pregen_tuples[rand()%Pregen_tuples_cnt];
	}
}
void dump(tIndividual * individual, ulong generation, tParams * params, int thread_id, char * tape_log, ulong restarts) {
	tTransTableItem * t = individual->table;
	int st, sy;
	unsigned long ulong_fit;
	char fname [255];
	FILE * f, *ft;

	if (individual->fitness < ULONG_MAX/1e9)
		ulong_fit=1e8*individual->fitness;
	else
		ulong_fit=ULONG_MAX;


	sprintf(fname, "%s/%.9lu-%d-%lu-%lu.gv",
			params->output, ulong_fit, thread_id, generation, restarts);
	if ( (f=fopen(fname, "w"))==NULL ||
		 (ft=fopen(strcat(fname,".txt"), "w"))==NULL) {
		fprintf(stderr, "Error: Can't open file for new graph, exiting.\n");
		exit(EXIT_FAILURE);
	}
	fprintf(f,
		"digraph \"Finite state machine, fitness=%.6lf, "
				  "population_size=%d, states=%d, symbols=%d, "
				  "best_cnt=%d, kids_cnt=%d\" {\n"
		"	rankdir=LR;\n"
		"	size=\"8,5\"\n"
		"S12 [shape=doublecircle];\n"
		"	node [shape = circle];\n",
		individual->fitness,
		params->population_size, params->states, params->symbols,
		params->best_cnt, params->kids_cnt
	);
	if (log_level>=LOG_BEST_1)
		printf("Fitness=%.6lf, thread_id=%d, generation=%lu, restarts=%lu\n",
				individual->fitness, thread_id, generation, restarts);
	for (st=0; st<params->states; st++)			// for all their states
		for (sy=0; sy<params->symbols; sy++, t++)	{// for all their symbols
			fprintf(ft, "{ %d, %d, %d },\n", t->state, t->symbol, t->shift);
			fprintf(f, "	S%d -> S%d [ label = \"%d / %d, %s\" ];\n",
						st, t->state, sy, t->symbol, shift2str(t->shift));
		}
	fprintf(f, "}\n");
	fprintf(ft, "Tape content:\n%s", tape_log);
	fclose(f);
	fclose(ft);
}
int evolve_turing(tParams * params, tTape * sample_tapes, int nr_of_tapes) {
	int thread_id, population_size=params->population_size,
		symbols=params->symbols,
		states=params->states;
	tTransTableItem population[population_size*symbols*states];
	tIndividual population_fitness [population_size],
				* parent, * new_kid_place;
	char tape_log[TAPE_LOG_SIZE]; // this is a bit unsafe - I should better calculate how big the log should be...
	tTransitions trans={states, symbols};
	pqueue_t * pqueue = pqueue_init(population_size);
	ulong generation=0, i, kid, new_pos,
			last_success_generation=0, restarts=0;
	//ulong best_cnt, kids_cnt ;
	double old_fitness;

	thread_id=omp_get_thread_num();

	
	if (population==NULL || population_fitness==NULL || pqueue==NULL) {
		fprintf(stderr, "Can't allocate memory for such a population size!\n");
		exit(-1);
	}

	init_evolution(states, symbols);

	generate_population(population, population_fitness, params);
	for (i=0; i<population_size; i++) {
		trans.table=population_fitness[i].table;
		population_fitness[i].fitness=eval_sorting_fitness_n_tapes(&trans, sample_tapes, nr_of_tapes, tape_log);
		pqueue_insert(pqueue, &population_fitness[i]);
	}
	while (1) {
		// for each of the best individuals in population:
		//best_cnt=nr_of_best(generation, population_size);
		for (i=1; i<params->best_cnt; i++)  {
			parent=pqueue_get(pqueue, i);	 // get the i-th top ranking individuals:
			old_fitness=parent->fitness;
			//kids_cnt=nr_of_kids(generation, i, population_size);
			for (kid=0; kid<params->kids_cnt; kid++) { // generate new kids:
				/** create new  mutation of the i-th parent
				 * and store it in place of one of the worst individual.
				 * Note: It can happen that the parent becomes the worst individual inside this loop,
				 * and thus will be replaced by one of its kids/mutations, but that's...life.
				 */
				new_kid_place=pqueue_get(pqueue, population_size);

				mutate(parent, new_kid_place, states, symbols);
				trans.table=new_kid_place->table;
				new_kid_place->fitness=eval_sorting_fitness_n_tapes(&trans, sample_tapes, nr_of_tapes, tape_log);
				new_pos=pqueue_priority_changed(pqueue, old_fitness, population_size);
				if (new_pos==1) {
					dump(new_kid_place, generation, params, thread_id, tape_log, restarts);
					last_success_generation=generation;
				}
			}
		}
		if (log_level>=LOG_BEST_1)
			printf("Generation %lu finished\n", generation);
		generation++;
		if (generation-last_success_generation > params->degeneration_cnt) {
			printf("Thread %d: point of degeneration reached. Generating the whole new population\n", thread_id);
			restarts++;
			last_success_generation=generation;
			pqueue_reset(pqueue);
			generate_population(population, population_fitness, params);
			for (i=0; i<population_size; i++) {
				trans.table=population_fitness[i].table;
				population_fitness[i].fitness=eval_sorting_fitness_n_tapes(&trans, sample_tapes, nr_of_tapes, tape_log);
				pqueue_insert(pqueue, &population_fitness[i]);
			}

		}
	}
}
