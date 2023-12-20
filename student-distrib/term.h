#ifndef TERM_GUARD
#define TERM_GUARD

#include "types.h"
#include "lib.h"

#define TERM_COLS 80
#define TERM_ROWS 24

#define PAN_ROWS 1

#define NUM_TTYS 3

// macro to print to terminal. use this instead of shitty printf
#define PRINTLN(msg)							\
	do {										\
		const char _[] = msg;					\
		terminal_write(100, _, strlen(_));		\
	} while (0)

typedef struct _input {
	// here's an idea: to avoid copying the buffer on every stroke:
	// on cursor movement, save the new location
	// once you perform a non-insert operation (movement, deletion, enter)
	// _then_ you "insert" your edits by copying the buffer
	// you would need to change `screen` though
#define INPUT_BUFFER_SIZE 128
	uint8_t buffer[INPUT_BUFFER_SIZE];
	uint8_t idx;
	bool eol;
} input;

typedef struct _cursor {
	uint8_t row;
	uint8_t col;
} cursor;

typedef struct _terminal {
	input input;
	cursor cursor;
	int32_t pid;
	uint8_t* mem;
} terminal;

extern terminal terms[3];
extern uint8_t scheduled_term;
extern uint8_t visible_term;

void terminal_clear();
void init_terms();

int32_t terminal_write(int32_t fd, const void *buf, int32_t nbytes);
int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes);
int32_t terminal_open(const uint8_t *filename);
int32_t terminal_close(int32_t fd);
void term_switch(uint32_t new_term);

#endif
