#include "kbd.h"
#include "term.h"
#include "signals.h"

// we need separate variables; consider the case where 2 of the same mod are held at the same time (but released separately). You could also use a counter, but bools are simpler
typedef struct kbd_mods {
	bool lshift;
	bool rshift;
	bool caps_lock;

	bool lcontrol;
	bool rcontrol;

	bool lalt;
	bool ralt;

	bool extended;
} _kbd_mods;
volatile _kbd_mods kbd_mods;

/* 
 *  keyboard_init
 *  DESCRIPTION: Initializes the PS/2 keyboard
 *  INPUTS: none
 *  OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: Enables IRQ 1
 */
void keyboard_init(void) {
    cli();
	// TODO set to scan mode 01 (on by default)
	/* outb(0xF0, KBD_COMMAND); */
	/* (void)inb(KBD_DATA); // ACK */
	/* outb(0x01, KBD_COMMAND); */
    sti();
    enable_irq(KBD_IRQ_NUM);
}

/* 
 *  keyboard_handler
 *  DESCRIPTION: Gets a key press and prints to screen.
 *               See ass_link_handler.S for assembly linkage
 *  INPUTS: none
 *  OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: Fetches a byte from PS/2 DATA buffer and clears interrupt
 */
//__attribute__ ((interrupt))
const static uint8_t bspace = '\b';
void keyboard_handler(void) {
	cli();
	uint8_t i;
	uint8_t chr = key_to_ascii(inb(KBD_DATA));
	terminal* term = &terms[visible_term];
	switch (chr) {
	// EXTRA CREDIT: Emacs keybinds
	case '\e':
	case '\0':
		break;
	case 0x3b:
	case 0x3c:
	case 0x3d:
		//PRINTLN("Multiple terms in progress...");
		term_switch((uint32_t)chr-0x3b);
		break;
	case '\n':
		// the general case should ensure input.idx never exceeds 126
		term->input.buffer[term->input.idx] = chr;
		terminal_write(100, (uint8_t*)(&term->input.buffer[term->input.idx++]), 1);
		term->input.eol = true;
		break;
	case '\t':
		for (i = 0; i < 4 && term->input.idx < INPUT_BUFFER_SIZE-2; i++) {
			term->input.buffer[term->input.idx] = ' ';
			terminal_write(100, (uint8_t*)(&term->input.buffer[term->input.idx++]), 1);
		}
		break;
	case '\b': // while it would be cool to simply move backwards, I think that would lose points
	case '\177':
		if (term->input.idx != 0) {
			term->input.buffer[--term->input.idx] = ' ';
			terminal_write(100, (uint8_t*)&bspace, 1); // move backwards
		}
		break;
	case 'c':
	case 'C':
	case 'l':
	case 'L': // this is not really extendable for other cases, but it works here!
		if ((kbd_mods.lcontrol || kbd_mods.rcontrol) && ((chr == 'l') || (chr == 'L'))) {
			clear_phys_screen();
			break;
		}
		if((kbd_mods.lcontrol || kbd_mods.rcontrol) && ((chr == 'C') || (chr == 'c'))){
			// ctrl+c pressed, signal 2
			send_signal(2);
			break;
		}
	default:
		if (term->input.idx < INPUT_BUFFER_SIZE-2) { // -1 because buffer always ends with a \n
			term->input.buffer[term->input.idx] = chr;
			terminal_write(100, (uint8_t*)(&term->input.buffer[term->input.idx++]), 1);
		}
	};
	send_eoi(KBD_IRQ_NUM);
	sti();
}

// I'm sure keyboards looked different back in the day,
// but I'm treating anything outside the "modern" style
// as an exception to the rule
const uint8_t row0[12] = "1234567890-=";
const uint8_t srow0[12] = "!@#$%^&*()_+";
const uint8_t row1[12] = "qwertyuiop[]";
const uint8_t srow1[12] = "QWERTYUIOP{}";
const uint8_t row2[11] = "asdfghjkl;'";
const uint8_t srow2[11] = "ASDFGHJKL:\"";
const uint8_t row3[10] = "zxcvbnm,./";
const uint8_t srow3[10] = "ZXCVBNM<>?";

/* key_to_ascii
 * This returns the ASCII represntation of a keycode (if applicable) or mutates `kbd_mods` to reflect a modifier key being active. Checking e.g. "Ctrl-L" must be done elsewhere.
 * Side effects: possibly mutates kbd_mods
 * Return: ASCII code of the logical key pressed, or the sentinel '0x00' if it's a modifier, extender, or an unsupported key.
 */
uint8_t key_to_ascii(uint8_t keycode) {
	// modifiers

	/* shift/capslock */
	if (keycode == 0x2A) {
		kbd_mods.lshift = true;
	} else if (keycode == 0xAA) {
		kbd_mods.lshift = false;
	} else if (keycode == 0x36) {
		kbd_mods.rshift = true;
	} else if (keycode == 0xB6) {
		kbd_mods.rshift = false;
	} else if (keycode == 0x3A) {
		kbd_mods.caps_lock = !kbd_mods.caps_lock;
	} else if (keycode == 0xBA) {
		// intentional nop
		//kbd_mods.caps_lock = false;
	/* ctrl */
	} else if (kbd_mods.extended && keycode == 0x1D) {
		kbd_mods.rcontrol = true;
	} else if (kbd_mods.extended && keycode == 0x9D) {
		kbd_mods.rcontrol = false;
	} else if (keycode == 0x1D) {
		kbd_mods.lcontrol = true;
	} else if (keycode == 0x9D) {
		kbd_mods.lcontrol = false;
	/* alt */
	} else if (kbd_mods.extended && keycode == 0x38) {
		kbd_mods.ralt = true;
	} else if (kbd_mods.extended && keycode == 0xB8) {
		kbd_mods.ralt = false;
	} else if (keycode == 0x38) {
		kbd_mods.lalt = true;
	} else if (keycode == 0xB8) {
		kbd_mods.lalt = false;
	}
	// end modifiers
	if (keycode == 0xE0) {
		kbd_mods.extended = true;
	} else {
		kbd_mods.extended = false;

		// modifier insensitive keys
		if (keycode == 0x0F) {
			return '\t';
		} else if (keycode == 0x1C) {
			return '\n'; // this might be wrong...
		} else if (keycode == 0x0E) {
			return '\b';
		} else if (keycode == 0x01) {
			return '\e'; // nonstandard but *probably* supported
		} else if (keycode == 0x39) {
			return ' ';
		}

		//the function keys should only work if alt is pressed
		if(keycode >= 0x3b && keycode <= 0x3d && (kbd_mods.lalt || kbd_mods.ralt)){
			return keycode;
		}


		// shiftable keys
		if ((kbd_mods.lshift || kbd_mods.rshift) ^ kbd_mods.caps_lock) {
			if (0x02 <= keycode && keycode <= 0x0D) {
				return srow0[keycode-0x02];
			} else if (0x10 <= keycode && keycode <= 0x1b) {
				return srow1[keycode-0x10];
			} else if (0x1E <= keycode && keycode <= 0x28) {
				return srow2[keycode-0x1E];
			} else if (0x2C <= keycode && keycode <= 0x35) {
				return srow3[keycode-0x2C];
			} else if (keycode == 0x29) {
				return '~';
			} else if (keycode == 0x2B) {
				return '|';
			}
		} else {
			if (0x02 <= keycode && keycode <= 0x0D) {
				return row0[keycode-0x02];
			} else if (0x10 <= keycode && keycode <= 0x1b) {
				return row1[keycode-0x10];
			} else if (0x1E <= keycode && keycode <= 0x28) {
				return row2[keycode-0x1E];
			} else if (0x2C <= keycode && keycode <= 0x35) {
				return row3[keycode-0x2C];
			} else if (keycode == 0x29) {
				return '`';
			} else if (keycode == 0x2B) {
				return '\\';
			}
		}
	}

	return '\0';
}

