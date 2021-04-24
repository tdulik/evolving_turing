#include <stdio.h>
#include <stdlib.h>
#include "turing.h"
#include "common.h"

tTransTableItem * getTransition(tTransitions * t, int state, signed char symbol) {
	if (state < t->states && symbol < t->symbols)
		return t->table + state*t->symbols + symbol;
	else {
		printf("Critical error!\nTrying to get state=%d, symbol=%d, but trans. table size has %d states and %d symbols\n", 
				state, symbol, t->states, t->symbols);
		exit(-1);
	}
}

char * shifts[] = {"R", "L", "RR", "N"};
#pragma omp threadprivate(shifts)
inline char * shift2str(tShift shift) {
	return shifts[shift];
}

void turing(tTape * tape, tTransitions * t, int max_steps, tStatus * status) {
	signed char symbol;			
	int head=1;		//turing read/write head position=2nd symbol (our tapes must begin with BLANK symbol)
	while (status->steps < max_steps && status->state < t->states) {
	tTransTableItem * trans;
		status->steps++;
		symbol=tape->content[head];				//this variable is good only for debugging...
		trans=getTransition(t, status->state, symbol);
		status->state=trans->state;
		if (trans->symbol >= 0) {
			tape->content[head]=trans->symbol;
			status->writes++;
			if (head>status->head_max) status->head_max=head;
		}
		switch (trans->shift) {
			case L: head--; break;
			case R: head++; break;
			case RR: head+=2; break;
		}
		if (head<0 || head>=TAPE_LEN) {
			if (log_level>=LOG_DEBUG_3) fprintf(stderr, "Head out of bounds!\n");
			status->error=ERR_BOUNDS;
			return;
		}
	} 					// while(...) - the main loop;
}
