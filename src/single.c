#include "single.h"

#include "tui.h"

void handle_single(struct oldteststate *os, chtype ch) {
	int update = 0;
	switch (ch) {
	case KEY_ESC:
		os->single = 0;
		break;
	case 'r':
		os->last_loaded = 0;
		update = 1;
	default:
		break;
	}

	if (update)
		draw_single(os, 1, 1);
}

void draw_single(struct oldteststate *os, int lazy, int load) {
}
