#ifndef TURING_H
#define TURING_H

enum { E=-1, BLANK};

enum {ERR_BOUNDS=-1};
typedef unsigned char uchar;
typedef signed char schar;
typedef unsigned long ulong;
typedef enum {R, L, RR, N} tShift;

#define SHIFTS 4

typedef struct {
		unsigned char state;
		signed char symbol;
		unsigned char shift;
}  tTransTableItem; //mathematically said, we define the codomain of the state transition function here

typedef struct {
	int	states; 	// nr. of states (including start, excluding "end" and "error")
	int symbols;	// nr. of input symbols (including BLANK, excluding "E"mpty)
	tTransTableItem * table;
} tTransitions;

#define TAPE_LEN 1000

typedef struct {
	schar content[TAPE_LEN];// array containing the tape symbols
	int input_len;			// length of the initial symbols sequence
	void * metrics;			// possibly any metrics of the tape content
} tTape;

typedef struct {
	int state, steps, writes, error, head_max;
} tStatus;

void turing(tTape * tape, tTransitions * t, int max_steps, tStatus * status);
inline char * shift2str(tShift shift);
#endif
