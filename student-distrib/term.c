#include "term.h"
#include "kbd.h"
#include "lib.h"
#include "paging.h"

#define UNPRINTABLE 0x06

/* the key intuition for `term` is that after PIT/paging is enabled, VIDMEM is always mapped to the SCHEDULED video page (which is in turn mapped to physical address b8000. */
/* because code only runs when its scheduled, VIDMEM always points to the appropriate page */
#define VIDMEM       0xB8000
#define VISIBLE_VIDMEM       0xB8000

terminal terms[NUM_TTYS];
uint8_t scheduled_term;
uint8_t visible_term;

static bool cursor_increment(terminal* term);
static void cursor_decrement(terminal* term);
static void cursor_increment_line(terminal* term);
static void pan(uint8_t rows, terminal *term);
static void update_cursor(terminal* term);

/* cursor_increment
 * Moves the cursor forward 1 "logical" character, including down a line if need be. Returns `True` if this is the case.
 * Side effects: Mutates `cursor`
 * Returns: True if cursor.row incremented
 */
static bool cursor_increment(terminal* term) {
	term->cursor.col = (term->cursor.col+1) % TERM_COLS; 
	if (term->cursor.col == 0) {
		term->cursor.row = (term->cursor.row+1) % TERM_ROWS; 
		if (term->cursor.row == 0) {
			pan(PAN_ROWS, term);
		}
		return true;
	}
	return false;
}

/* cursor_decrement
 * Goes down (or rather up) a line, for example when pressing backspace.
 * Side effects: Mutates `cursor`
 * Returns: None
 */
static void cursor_decrement(terminal* term) {
	if (term->cursor.col == 0) {
		term->cursor.col = TERM_COLS-1; term->cursor.row--;
	} else {
		term->cursor.col--;
	}
}

/* cursor_increment_line
 * Goes up (or rather down) a line, for example when pressing enter.
 * Side effects: Mutates `cursor`
 * Returns: None
 */
static void cursor_increment_line(terminal* term) {
	term->cursor.col = 0; term->cursor.row++;
	if ((term->cursor.row %= TERM_ROWS) == 0)
		pan(PAN_ROWS, term);
}

/* pan
 * "Pans" the screen up by `rows` rows. You can pan up TERM_ROWS # of rows to completely refresh the screen, or (currently FUBAR) only scroll up a few lines, updating `screen` and video memory in the process.
 * Side effects: Writes to video memory
 * Returns: None
 */
static const uint8_t whiteout[160] = " \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a \x0a";
static void pan(uint8_t rows, terminal *term) {
	int i;
	uint8_t* mem = (term == &terms[visible_term]) ? (uint8_t*)0xb7000 : term->mem;
	// we could switch back to the old memcpy model if we wrote directly to vid pages.
	// there would (theoretically) be no need for a critical section. need to await
	// approval from the paging gods on this one
	memmove(mem, mem+rows*TERM_COLS*2, 2*(TERM_ROWS-rows)*TERM_COLS*sizeof(uint8_t));
	for (i = 0; i < rows; i++)
		memcpy(mem+(TERM_ROWS-1-i)*TERM_COLS*2, (const void*)whiteout, 2*TERM_COLS*sizeof(uint8_t));
	pair _p = { .x = 0, .y = TERM_ROWS-rows };
	_screen_seek(_p);
	term->cursor.row = TERM_ROWS-rows; term->cursor.col = 0;
}

/* terminal_clear
 * Clears the screen using `pan`, for use with Ctrl-L.
 * Mutates video memory, returns nothing
 */
void terminal_clear() {
	// this is always called to clear what's on screen
	terminal *term = &terms[visible_term];
	pan(PAN_ROWS, term);
	update_cursor(term);
}

/* terminal_write
 * Writes to `screen` and then draws to video memory using `terminal_print`.
 * Side effects: Writes to video memory, mutates `screen` (only `len` bytes).
 * Returns: None
 * We will purify this world of impurities. How, you may ask? Simple, my friend.
 * Zig.
 */
int32_t terminal_write(int32_t fd, const void *buf, int32_t nbytes) {
	cli();
	if ((scheduled_term != visible_term) && (page_table_0[0xb8].addr_31_12 == 0xb8)) {
        page_table_0[0xb8].addr_31_12 = 0xb9+scheduled_term;
        flush_tlb();
    }

	int i;
	terminal *term = &terms[scheduled_term];
	const uint8_t* chrs = (const uint8_t*)buf;
	uint8_t color = ATTRIB;
	// tl;dr when called from an interrupt context the 2nd condiiton is necessarily true,
	// so we can use the "useless" fd parameter. Very clever, aadi.
	bool visible = (fd == 100);// || (scheduled_term == visible_term);
	if (visible)
		term = &terms[visible_term];
	(void)fd;

	if (buf == NULL) return -1;
	pair p;

	for (i = 0; i < nbytes; i++) {
		if (chrs[i] == '\b') {
			cursor_decrement(term); // the decrement comes first here
			p.x = term->cursor.col; p.y = term->cursor.row;
			_screen_seek(p);
			if (visible) screen_putc(' ', color);
			else putc(' ', color);
			_screen_seek(p);
		} else if (chrs[i] == '\n' || chrs[i] == '\r') {
			p.x = term->cursor.col; p.y = term->cursor.row;
			_screen_seek(p);
			if (visible) screen_putc('\n', color);
			else putc('\n', color);
			cursor_increment_line(term);
		} else if(chrs[i] == '\0') {
			continue;
		} else if (chrs[i] == '\t') {
			p.x = term->cursor.col; p.y = term->cursor.row;
			_screen_seek(p);
			if (visible) screen_putc(UNPRINTABLE, color);
			else putc(UNPRINTABLE, color);
			cursor_increment(term);
		/* } else if (chrs[i] == SPECIAL_COLOR_BYTE) { we can use this to change colors based on input! */
		} else {
			p.x = term->cursor.col; p.y = term->cursor.row;
			_screen_seek(p);
			if (visible) screen_putc(chrs[i], color);
			else putc(chrs[i], color);
			cursor_increment(term);
		}
	}
	if (visible || scheduled_term == visible_term)
		update_cursor(term);
	sti();

	return nbytes;
}

/* terminal_read
 * Used to read a line of input from a terminal. Once enter is pressed, copies `nbytes` of `input.buffer` into user-provided `buf` and returns the num of bytes read (i.e. min(input.idx, nbytes)).
 * Side effects: Writes to `buf`
 * Returns: Number of bytes read
 */
int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes) {
	uint16_t len;
	terminal *term = &terms[scheduled_term];
	(void)fd;
	if (buf == NULL || nbytes < 0) return -1;
	sti();
	while (!term->input.eol) ;
	cli();
	len = term->input.idx < nbytes ? term->input.idx : nbytes;
	memcpy(buf, (const void*)term->input.buffer, len);
	term->input.eol = false;
	term->input.idx = 0;
	return len;
}

int32_t terminal_open(const uint8_t *filename) {
	(void)filename;
	return 0;
}

int32_t terminal_close(int32_t fd) {
	(void)fd;
	return 0;
}

/* init_terms
 * Most stuff is 0-inited, we just set the PIDs to -1
 * This _does not_ zero vid pages for the respective tty
 */
void init_terms() {
	int i, j;
	clear_phys_screen();
	for (i = 0; i < NUM_TTYS; i++) {
		terms[i].pid = -1;
		terms[i].mem = (uint8_t*) (0xB9000+0x1000*i);
		for (j = 0; j < TERM_ROWS; j++)
			memcpy(terms[i].mem+(TERM_ROWS-1-j)*TERM_COLS*2, (const void*)whiteout, 2*TERM_COLS*sizeof(uint8_t));
	}

	status_puts("Terminal ", 0);
	visible_term = 0;
	scheduled_term = 2;
	*(uint8_t*)(0xB7000 + (24*80 + 9) * 2) = 0x30 + visible_term;
}

/* 
 *  term_switch
 *  DESCRIPTION: changes the terminal that is being displayed on screen
 *  INPUTS: new_term, a number in range [0,NUM_TTYS-1] corresponding to terminal we want to switch to. Zero indexed
 *  OUTPUTS: Shows us the terminal we want to see on screen
 *  RETURN VALUE: None
 */
void term_switch(uint32_t new_term){
	cli();
	if (new_term == visible_term) return;
	// page_table_0[VIDEO_MEM_ADDR].addr_31_12 = VIDEO_MEM_ADDR;
	// flush_tlb();
	//using paging
	memcpy((void*)((0xb9+visible_term) << 12),(const void*)0xB7000,NUM_COLS*NUM_ROWS*2);
	clear_phys_screen();
	memcpy((void*)0xB7000,(const void*)((0xb9+new_term) << 12),NUM_COLS*NUM_ROWS*2);
	visible_term = new_term;
	update_cursor(&terms[new_term]);
	// if(scheduled_term != visible_term){page_table_0[VIDEO_MEM_ADDR].addr_31_12 = 0xb9 + scheduled_term;}
	// flush_tlb();
	*(uint8_t*)(0xB7000 + (24*80 + 9) * 2) = 0x30 + visible_term;
	sti();
	//update_cursor(&(terms[new_term].cursor));
}


/* update_cursor
 * Pulled straight from OSDev; draws the cursor according to the global variable `cursor`.
 * Side effects: Communicates with cursor "hardware"
 * Returns: None
 */
#define TERM_CURSOR_COMMAND 0x3D4
#define TERM_CURSOR_DATA (TERM_CURSOR_COMMAND+1)
#define TERM_CURSOR_SETLOW 0x0F
#define TERM_CURSOR_SETHIGH (TERM_CURSOR_SETLOW-1)
static void update_cursor(terminal* term) {
	cli();
	uint16_t pos = term->cursor.row * TERM_COLS + term->cursor.col;
 
	outb(TERM_CURSOR_SETLOW, TERM_CURSOR_COMMAND);
	outb((uint8_t) (pos & 0xFF), TERM_CURSOR_DATA);
	outb(TERM_CURSOR_SETHIGH, TERM_CURSOR_COMMAND);
	outb((uint8_t) ((pos >> 8) & 0xFF), TERM_CURSOR_DATA);
	sti();
}
